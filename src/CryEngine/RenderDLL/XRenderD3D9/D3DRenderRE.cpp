/*=============================================================================
  D3DRenderRE.cpp : implementation of the Rendering RenderElements pipeline.
  Copyright 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "../Common/RendElements/Stars.h"
#include "I3DEngine.h"

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
#include "IPhysicsGPU.h" 
#endif

#pragma warning(disable: 4244)


//enable once water update is used on PS3, currently no vertex animation possible







//=======================================================================

bool CRESky::mfDraw(CShader *ef, SShaderPass *sfm)
{
  CD3D9Renderer *rd = gcpRendD3D;

  if(!rd->m_RP.m_pShaderResources || !rd->m_RP.m_pShaderResources->m_pSky)
  {
    return false;
  }

  int bPrevClipPl = rd->m_RP.m_ClipPlaneEnabled;

  if(bPrevClipPl)
  {
    rd->FX_SetClipPlane(false, NULL, false);
  }

  // pass 0 - skybox
	SSkyInfo *pSky = rd->m_RP.m_pShaderResources->m_pSky;
  if(!pSky->m_SkyBox[0])
  {
    if(bPrevClipPl)
    {
      rd->FX_SetClipPlane(true, &rd->m_RP.m_CurClipPlane.m_Normal.x, rd->m_RP.m_bClipPlaneRefract);
    }

    return false;
  }

	float v(gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKYBOX_MULTIPLIER));
  rd->SetMaterialColor(v, v, v, m_fAlpha);

	if(!sfm)
  {
    ef->FXSetTechnique(NULL);
  }


  uint32 nPasses = 0;
  ef->FXBegin(&nPasses, FEF_DONTSETTEXTURES );
	//ef->FXBegin(&nPasses, 0 );
  if(!nPasses)
  {
    return false;
  }
	ef->FXBeginPass( 0 );

  rd->FX_PushVP();
	rd->m_NewViewport.fMinZ = 0.99f;
  rd->m_bViewportDirty = true;

  STexState pTexState;
  pTexState.SetFilterMode(FILTER_LINEAR);        
  pTexState.SetClampMode(1, 1, 1);

	int texStateID = CTexture::GetTexState(pTexState);

  const float fSkyBoxSize = SKY_BOX_SIZE;

	if (rd->m_RP.m_nBatchFilter & FB_Z)
	{
		CTexture::s_ptexBlack->Apply(0, texStateID);
		{ // top
			SVF_P3F_C4B_T2F data[] = 
			{
				{Vec3(+fSkyBoxSize, -fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{Vec3(-fSkyBoxSize, -fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{Vec3(+fSkyBoxSize, +fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{Vec3(-fSkyBoxSize, +fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0, 0)}
			};
			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);
			rd->DrawPrimitives(&vertexBuffer,4);
		}
		{ // nesw
			SVF_P3F_C4B_T2F data[] = 
			{ 
				{ Vec3(-fSkyBoxSize, -fSkyBoxSize, +fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(-fSkyBoxSize, -fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(+fSkyBoxSize, -fSkyBoxSize, +fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(+fSkyBoxSize, -fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(+fSkyBoxSize, +fSkyBoxSize, +fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(+fSkyBoxSize, +fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(-fSkyBoxSize, +fSkyBoxSize, +fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(-fSkyBoxSize, +fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(-fSkyBoxSize, -fSkyBoxSize, +fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{ Vec3(-fSkyBoxSize, -fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
			};
			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);	
			rd->DrawPrimitives(&vertexBuffer,10);
		}
		{	// b
			SVF_P3F_C4B_T2F data[] = 
			{
				{Vec3(+fSkyBoxSize, -fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{Vec3(-fSkyBoxSize, -fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{Vec3(+fSkyBoxSize, +fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)},
				{Vec3(-fSkyBoxSize, +fSkyBoxSize, -fSkyBoxSize), {{0}}, Vec2(0, 0)}
			};
			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);
			rd->DrawPrimitives(&vertexBuffer,4);
		}
	}
	else
	{
		{ // top
			SVF_P3F_C4B_T2F data[] = 
			{
				{Vec3(fSkyBoxSize,-fSkyBoxSize, fSkyBoxSize),  {{0}}, Vec2(1, 1.f-1)},
				{Vec3(-fSkyBoxSize,-fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0, 1.f-1)},
				{Vec3(fSkyBoxSize, fSkyBoxSize, fSkyBoxSize),  {{0}}, Vec2(1, 1.f-0)},
				{Vec3(-fSkyBoxSize, fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0, 1.f-0)}
			};

			((CTexture*)(pSky->m_SkyBox[2]))->Apply(0, texStateID);
			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);
			rd->DrawPrimitives(&vertexBuffer,4);
		}

		Vec3 camera = iSystem->GetViewCamera().GetPosition();
		camera.z = max(0.f,camera.z);

		float fWaterCamDiff = max(0.f,camera.z-m_fTerrainWaterLevel);

		float fMaxDist = gEnv->p3DEngine->GetMaxViewDistance()/1024.f;
		float P = (fWaterCamDiff)/128 + max(0.f,(fWaterCamDiff)*0.03f/fMaxDist);

		P *= m_fSkyBoxStretching;

		float D = (fWaterCamDiff)/10.0f*fSkyBoxSize/124.0f - /*P*/0 + 8;

		D = max(0.f,D);

		if(m_fTerrainWaterLevel>camera.z && SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID]==1)
		{
			P = (fWaterCamDiff);
			D = (fWaterCamDiff);
		}
	
		float fTexOffset;
		fTexOffset = 1.0f / pSky->m_SkyBox[1]->GetHeight();
		{ // s
			SVF_P3F_C4B_T2F data[] = 
			{ 
				{ Vec3(-fSkyBoxSize,-fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(1.0, 1.f-1.0) },
				{ Vec3(fSkyBoxSize,-fSkyBoxSize, fSkyBoxSize),  {{0}}, Vec2(0.0, 1.f-1.0) },
				{ Vec3(-fSkyBoxSize,-fSkyBoxSize,-P),           {{0}}, Vec2(1.0, 1.f-0.5-fTexOffset) },
				{ Vec3(fSkyBoxSize,-fSkyBoxSize,-P),            {{0}}, Vec2(0.0, 1.f-0.5-fTexOffset) },
				{ Vec3(-fSkyBoxSize,-fSkyBoxSize,-D),           {{0}}, Vec2(1.0, 1.f-0.5-fTexOffset) },
				{ Vec3(fSkyBoxSize,-fSkyBoxSize,-D),            {{0}}, Vec2(0.0, 1.f-0.5-fTexOffset) }
			};

			((CTexture*)(pSky->m_SkyBox[1]))->Apply(0, texStateID);
			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);
			rd->DrawPrimitives(&vertexBuffer,6);
		}
		{ // e
			SVF_P3F_C4B_T2F data[] = 
			{ 
				{ Vec3(-fSkyBoxSize, fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(1.0, 1.f-0.0) },
				{ Vec3(-fSkyBoxSize,-fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0.0, 1.f-0.0) },
				{ Vec3(-fSkyBoxSize, fSkyBoxSize,-P),           {{0}}, Vec2(1.0, 1.f-0.5f+fTexOffset) },
				{ Vec3(-fSkyBoxSize,-fSkyBoxSize,-P),           {{0}}, Vec2(0.0, 1.f-0.5f+fTexOffset) },
				{ Vec3(-fSkyBoxSize, fSkyBoxSize,-D),           {{0}}, Vec2(1.0, 1.f-0.5f+fTexOffset) },
				{ Vec3(-fSkyBoxSize,-fSkyBoxSize,-D),           {{0}}, Vec2(0.0, 1.f-0.5f+fTexOffset) }
			};

			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);
			rd->DrawPrimitives(&vertexBuffer,6);
		}

		fTexOffset = 1.0f / pSky->m_SkyBox[0]->GetHeight();
		{ // n
			SVF_P3F_C4B_T2F data[] = 
			{ 
				{ Vec3(fSkyBoxSize, fSkyBoxSize, fSkyBoxSize),  {{0}}, Vec2(1.0, 1.f-1.0) },
				{ Vec3(-fSkyBoxSize, fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0.0, 1.f-1.0) },
				{ Vec3(fSkyBoxSize, fSkyBoxSize,-P),            {{0}}, Vec2(1.0, 1.f-0.5-fTexOffset) },
				{ Vec3(-fSkyBoxSize, fSkyBoxSize,-P),           {{0}}, Vec2(0.0, 1.f-0.5-fTexOffset) },
				{ Vec3(fSkyBoxSize, fSkyBoxSize,-D),            {{0}}, Vec2(1.0, 1.f-0.5-fTexOffset) },
				{ Vec3(-fSkyBoxSize, fSkyBoxSize,-D),           {{0}}, Vec2(0.0, 1.f-0.5-fTexOffset) }
			};

			((CTexture*)(pSky->m_SkyBox[0]))->Apply(0, texStateID);
			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);
			rd->DrawPrimitives(&vertexBuffer,6);
		}
		{ // w
			SVF_P3F_C4B_T2F data[] = 
			{ 
				{ Vec3(fSkyBoxSize,-fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(1.0, 1.f-0.0) },
				{ Vec3(fSkyBoxSize, fSkyBoxSize, fSkyBoxSize), {{0}}, Vec2(0.0, 1.f-0.0) },
				{ Vec3(fSkyBoxSize,-fSkyBoxSize,-P),           {{0}}, Vec2(1.0, 1.f-0.5f+fTexOffset) },
				{ Vec3(fSkyBoxSize, fSkyBoxSize,-P),           {{0}}, Vec2(0.0, 1.f-0.5f+fTexOffset) },
				{ Vec3(fSkyBoxSize,-fSkyBoxSize,-D),           {{0}}, Vec2(1.0, 1.f-0.5f+fTexOffset) },
				{ Vec3(fSkyBoxSize, fSkyBoxSize,-D),           {{0}}, Vec2(0.0, 1.f-0.5f+fTexOffset) }
			};
			CVertexBuffer vertexBuffer(data,eVF_P3F_C4B_T2F);	
			rd->DrawPrimitives(&vertexBuffer,6);
		}
	}

  //DrawFogLayer();
  //DrawBlackPortal();

  if(bPrevClipPl)
  {
    rd->FX_SetClipPlane(true, &rd->m_RP.m_CurClipPlane.m_Normal.x, rd->m_RP.m_bClipPlaneRefract);
  }

  rd->FX_PopVP();
  rd->FX_ResetPipe(); 

  return true;
}

static void FillSkyTextureData(CTexture* pTexture, const void* pData, uint32 width, uint32 height, uint32 pitch, void* pTmpStorage)
{
	assert(pTexture);
#if (!defined(DIRECT3D10) || defined(PS3)) && !defined(XENON)
	(pTmpStorage); // TODO: use platform independent way to mark as unused (GCC?)
	CDeviceTexture* pDevTex = pTexture->GetDevTexture();
	assert(pDevTex);
	STexLock rect;
	if (SUCCEEDED(pDevTex->LockRect(0, rect, LF_DISCARD)))
	{
		for (uint32 h(0); h<height; ++h)
		{
			assert(pitch == 4 * sizeof(f32) * width );
			const f32* pSrc((const f32*)((size_t)pData + h * pitch));
			D3DXFLOAT16* pDst((D3DXFLOAT16*)((size_t)rect.pData + h * rect.Pitch));
			D3DXFloat32To16Array(pDst, pSrc, 4 * width);
		}
		pDevTex->UnlockRect(0);
	}
#elif defined(DIRECT3D10) || defined(XENON)
	assert(pTmpStorage);
	for (uint32 h(0); h<height; ++h)
	{
		assert(pitch == 4 * sizeof(f32) * width );
		const f32* pSrc((const f32*)((size_t)pData + h * pitch));
		D3DXFLOAT16* pDst((D3DXFLOAT16*)pTmpStorage + h * width * 4);
		D3DXFloat32To16Array(pDst, pSrc, 4 * width);
	}
#	if defined(DIRECT3D10)
	CDeviceTexture* pDevTex = pTexture->GetDevTexture();
	assert(pDevTex);
	gcpRendD3D->m_pd3dDeviceContext->UpdateSubresource(pDevTex->Get2DTexture(), 0, 0, pTmpStorage, 4 * width * sizeof(D3DXFLOAT16), 4 * height * width * sizeof(D3DXFLOAT16));
#else
	CDeviceTexture* pDevTex = pTexture->GetDevTexture();
	assert(pDevTex);
	STexLock rect;
	if (SUCCEEDED(pDevTex->LockRect(0, rect, LF_DISCARD)))
	{
		uint32 nFlags = 0;
		if(FALSE == XGIsPackedTexture(pDevTex->Get2DTexture()))
			nFlags |= XGTILE_NONPACKED;
		if(TRUE  == XGIsBorderTexture(pDevTex->Get2DTexture()))
			nFlags |= XGTILE_BORDER;
		XGTileTextureLevel(width, height, 0, XGGetGpuFormat(D3DFMT_A16B16G16R16F), nFlags, rect.pData, 0, pTmpStorage, sizeof(D3DXFLOAT16) * width * 4, 0);
		pDevTex->UnlockRect(0);
	}
#	endif
#else
	assert(0);
#endif
}

static void GenerateSkyDomeTextures(CTexture*& pSkyDomeTextureMie, CTexture*& pSkyDomeTextureRayleigh, uint32 width, uint32 height)
{
	SAFE_RELEASE(pSkyDomeTextureMie);
	SAFE_RELEASE(pSkyDomeTextureRayleigh);

	int creationFlags = FT_STATE_CLAMP | FT_DONT_RELEASE | FT_NOMIPS;	
#if !defined(DIRECT3D10) || defined(PS3)
	creationFlags |= FT_USAGE_DYNAMIC;
#endif

	if( gcpRendD3D->m_bDeviceSupportsFP16Filter )
		creationFlags |= FT_FILTER_LINEAR;

	pSkyDomeTextureMie = CTexture::Create2DTexture("$SkyDomeTextureMie", width, height, 1, creationFlags, 0, eTF_A16B16G16R16F, eTF_A16B16G16R16F);
	pSkyDomeTextureMie->SetCustomID(TO_SKYDOME_MIE);
	pSkyDomeTextureMie->Fill(ColorF(0, 0, 0, 0));
	pSkyDomeTextureMie->SetFilterMode(gcpRendD3D->m_bDeviceSupportsFP16Filter ? FILTER_LINEAR : FILTER_POINT);
	pSkyDomeTextureMie->SetClampingMode(0, 1, 1);
	pSkyDomeTextureMie->UpdateTexStates();

	pSkyDomeTextureRayleigh = CTexture::Create2DTexture("$SkyDomeTextureRayleigh", width, height, 1, creationFlags, 0, eTF_A16B16G16R16F, eTF_A16B16G16R16F);
	pSkyDomeTextureRayleigh->SetCustomID(TO_SKYDOME_RAYLEIGH);
	pSkyDomeTextureRayleigh->Fill(ColorF(0, 0, 0, 0));
	pSkyDomeTextureRayleigh->SetFilterMode(gcpRendD3D->m_bDeviceSupportsFP16Filter ? FILTER_LINEAR : FILTER_POINT);
	pSkyDomeTextureRayleigh->SetClampingMode(0, 1, 1);
	pSkyDomeTextureRayleigh->UpdateTexStates();
}

bool CREHDRSky::mfDraw( CShader *ef, SShaderPass *sfm )
{
	CD3D9Renderer* rd( gcpRendD3D );

	if( !rd->m_RP.m_pShaderResources || !rd->m_RP.m_pShaderResources->m_pSky )
		return false;

	int bPrevClipPl( rd->m_RP.m_ClipPlaneEnabled );
	if( bPrevClipPl )
		rd->FX_SetClipPlane( false, 0, false );

	SSkyInfo* pSky( rd->m_RP.m_pShaderResources->m_pSky );
	if( !pSky->m_SkyBox[0] )
	{
		if( bPrevClipPl )
			rd->FX_SetClipPlane( true, &rd->m_RP.m_CurClipPlane.m_Normal.x, rd->m_RP.m_bClipPlaneRefract );

		return false;
	}

	assert( m_pRenderParams );
	if( !m_pRenderParams )
		return false;

	bool isNotZPass = (rd->m_RP.m_nBatchFilter & FB_Z) == 0;

	if (isNotZPass)
	{
		// re-create sky dome textures if necessary
		bool forceTextureUpdate(false);
    if (!CTexture::IsTextureExist(m_pSkyDomeTextureMie) ||
				m_pSkyDomeTextureMie->GetWidth() != m_pRenderParams->m_skyDomeTextureWidth ||
				m_pSkyDomeTextureMie->GetHeight() != m_pRenderParams->m_skyDomeTextureHeight ||
				!CTexture::IsTextureExist(m_pSkyDomeTextureRayleigh) ||
				m_pSkyDomeTextureRayleigh->GetWidth() != m_pRenderParams->m_skyDomeTextureWidth ||
				m_pSkyDomeTextureRayleigh->GetHeight() != m_pRenderParams->m_skyDomeTextureHeight)
		{
			GenerateSkyDomeTextures(m_pSkyDomeTextureMie, m_pSkyDomeTextureRayleigh, m_pRenderParams->m_skyDomeTextureWidth, m_pRenderParams->m_skyDomeTextureHeight);
			forceTextureUpdate = true;			
#if (defined(DIRECT3D10) && !defined(PS3)) || defined(XENON)
			SAFE_DELETE_ARRAY(m_pTmpStorage);
			m_pTmpStorage = new unsigned char[4 * sizeof(D3DXFLOAT16) * m_pRenderParams->m_skyDomeTextureWidth * m_pRenderParams->m_skyDomeTextureHeight];
#endif
		}

		// dyn tex data lost due to device reset?
		if (m_frameReset != rd->m_nFrameReset)
		{
			forceTextureUpdate = true;
			m_frameReset = rd->m_nFrameReset;
		}

		// update sky dome texture if new data is available
		if (m_skyDomeTextureLastTimeStamp != m_pRenderParams->m_skyDomeTextureTimeStamp || forceTextureUpdate)
		{
			FillSkyTextureData(m_pSkyDomeTextureMie, m_pRenderParams->m_pSkyDomeTextureDataMie, 
				m_pRenderParams->m_skyDomeTextureWidth, m_pRenderParams->m_skyDomeTextureHeight, m_pRenderParams->m_skyDomeTexturePitch, m_pTmpStorage);

			FillSkyTextureData(m_pSkyDomeTextureRayleigh, m_pRenderParams->m_pSkyDomeTextureDataRayleigh, 
				m_pRenderParams->m_skyDomeTextureWidth, m_pRenderParams->m_skyDomeTextureHeight, m_pRenderParams->m_skyDomeTexturePitch, m_pTmpStorage);

			// update time stamp of last update
			m_skyDomeTextureLastTimeStamp = m_pRenderParams->m_skyDomeTextureTimeStamp;
		}
	}

	// render
	uint32 nPasses( 0 );
	ef->FXBegin( &nPasses, 0 );
	if( !nPasses )
		return false;
	ef->FXBeginPass( 0 );

	I3DEngine* p3DEngine( gEnv->p3DEngine );

	rd->FX_PushVP();
	rd->m_NewViewport.fMinZ = 0.99f;
	rd->m_bViewportDirty = true;

	if (isNotZPass)
	{
		// shader constants -- set sky dome texture and texel size
		assert(m_pSkyDomeTextureMie && m_pSkyDomeTextureMie->GetWidth() == m_pRenderParams->m_skyDomeTextureWidth && m_pSkyDomeTextureMie->GetHeight() == m_pRenderParams->m_skyDomeTextureHeight);
		assert(m_pSkyDomeTextureRayleigh && m_pSkyDomeTextureRayleigh->GetWidth() == m_pRenderParams->m_skyDomeTextureWidth && m_pSkyDomeTextureRayleigh->GetHeight() == m_pRenderParams->m_skyDomeTextureHeight);
		Vec4 skyDomeTexSizeVec( (float) m_pRenderParams->m_skyDomeTextureWidth, (float) m_pRenderParams->m_skyDomeTextureHeight, 0.0f, 0.0f );
		static CCryName Param1Name("SkyDome_TextureSize");
		ef->FXSetPSFloat(Param1Name, &skyDomeTexSizeVec, 1 );
		Vec4 skyDomeTexelSizeVec( 1.0f / (float) m_pRenderParams->m_skyDomeTextureWidth, 1.0f / (float) m_pRenderParams->m_skyDomeTextureHeight, 0.0f, 0.0f );
		static CCryName Param2Name("SkyDome_TexelSize");
		ef->FXSetPSFloat(Param2Name, &skyDomeTexelSizeVec, 1 );

		// shader constants -- phase function constants
		static CCryName Param3Name("SkyDome_PartialMieInScatteringConst");
		static CCryName Param4Name("SkyDome_PartialRayleighInScatteringConst");
		static CCryName Param5Name("SkyDome_SunDirection");
		static CCryName Param6Name("SkyDome_PhaseFunctionConstants");
		ef->FXSetPSFloat(Param3Name, &m_pRenderParams->m_partialMieInScatteringConst, 1 );
		ef->FXSetPSFloat(Param4Name, &m_pRenderParams->m_partialRayleighInScatteringConst, 1 );
		ef->FXSetPSFloat(Param5Name, &m_pRenderParams->m_sunDirection, 1 );
		ef->FXSetPSFloat(Param6Name, &m_pRenderParams->m_phaseFunctionConsts, 1 );

		// shader constants -- night sky relevant constants
		Vec3 nightSkyHorizonCol;
		p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_HORIZON_COLOR, nightSkyHorizonCol );
		Vec3 nightSkyZenithCol;
		p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_ZENITH_COLOR, nightSkyZenithCol );
		float nightSkyZenithColShift( p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_ZENITH_SHIFT ) );	
		const float minNightSkyZenithGradient( -0.1f );

		static CCryName Param7Name("SkyDome_NightSkyColBase");
		static CCryName Param8Name("SkyDome_NightSkyColDelta");
		static CCryName Param9Name("SkyDome_NightSkyZenithColShift");

		Vec4 nsColBase( nightSkyHorizonCol, 0 );
		ef->FXSetPSFloat(Param7Name, &nsColBase, 1 );
		Vec4 nsColDelta( nightSkyZenithCol - nightSkyHorizonCol, 0 );
		ef->FXSetPSFloat(Param8Name, &nsColDelta, 1 );
		Vec4 nsZenithColShift( 1.0f / ( nightSkyZenithColShift - minNightSkyZenithGradient ),  -minNightSkyZenithGradient / ( nightSkyZenithColShift - minNightSkyZenithGradient ) , 0, 0 );
		ef->FXSetPSFloat(Param9Name, &nsZenithColShift, 1 );

		Vec3 nightMoonDirection;
		p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_MOON_DIRECTION, nightMoonDirection );
		float nightMoonSize( 25.0f - 24.0f * clamp_tpl( p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_MOON_SIZE ), 0.0f, 1.0f ) );
		Vec4 nsMoonDirSize( nightMoonDirection, nightMoonSize );
		static CCryName Param10Name("SkyDome_NightMoonDirSize");
		ef->FXSetVSFloat(Param10Name, &nsMoonDirSize, 1 );
		ef->FXSetPSFloat(Param10Name, &nsMoonDirSize, 1 );

		Vec3 nightMoonColor;
		p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_MOON_COLOR, nightMoonColor );
		Vec4 nsMoonColor( nightMoonColor, 0 );
		static CCryName Param11Name("SkyDome_NightMoonColor");
		ef->FXSetPSFloat(Param11Name, &nsMoonColor, 1 );

		Vec3 nightMoonInnerCoronaColor;
		p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_MOON_INNERCORONA_COLOR, nightMoonInnerCoronaColor );
		float nightMoonInnerCoronaScale( 1.0f + 1000.0f * p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_MOON_INNERCORONA_SCALE ) );	
		Vec4 nsMoonInnerCoronaColorScale( nightMoonInnerCoronaColor, nightMoonInnerCoronaScale );
		static CCryName Param12Name("SkyDome_NightMoonInnerCoronaColorScale");
		ef->FXSetPSFloat(Param12Name, &nsMoonInnerCoronaColorScale, 1 );

		Vec3 nightMoonOuterCoronaColor;
		p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_MOON_OUTERCORONA_COLOR, nightMoonOuterCoronaColor );
		float nightMoonOuterCoronaScale( 1.0f + 1000.0f * p3DEngine->GetGlobalParameter( E3DPARAM_NIGHSKY_MOON_OUTERCORONA_SCALE ) );
		Vec4 nsMoonOuterCoronaColorScale( nightMoonOuterCoronaColor, nightMoonOuterCoronaScale );
		static CCryName Param13Name("SkyDome_NightMoonOuterCoronaColorScale");
		ef->FXSetPSFloat(Param13Name, &nsMoonOuterCoronaColorScale, 1 );
	}

	HRESULT hr(S_OK);

	// commit all render changes
	rd->FX_Commit();

	// set vertex declaration and streams of sky dome
  CRenderMesh2* pSkyDomeMesh((CRenderMesh2*)m_pRenderParams->m_pSkyDomeMesh);
	hr = rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);
  if (!FAILED(hr))
  {
	  // set vertex and index buffer
	  pSkyDomeMesh->CheckUpdate(pSkyDomeMesh->_GetVertexFormat(), 0);
    int vbOffset(0);
    int ibOffset(0);
    D3DVertexBuffer *pVB = rd->m_DevBufMan.GetD3DVB(pSkyDomeMesh->_GetVBStream(VSF_GENERAL), &vbOffset);
    D3DIndexBuffer *pIB = rd->m_DevBufMan.GetD3DIB(pSkyDomeMesh->_GetIBStream(), &ibOffset);
    assert(pVB);
    assert(pIB);
		if(!pVB || !pIB)
			return false;

	  hr =  rd->FX_SetVStream(0, pVB, vbOffset, pSkyDomeMesh->GetStreamStride(VSF_GENERAL));

	  ibOffset /= sizeof(uint16);
	  hr = rd->FX_SetIStream(pIB);

	  // draw sky dome
  #if defined (DIRECT3D9) || defined(OPENGL)
	  hr = rd->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pSkyDomeMesh->_GetNumVerts(), ibOffset, pSkyDomeMesh->_GetNumInds() / 3);
  #elif defined (DIRECT3D10)
	  rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	  rd->m_pd3dDeviceContext->DrawIndexed(pSkyDomeMesh->_GetNumInds(), ibOffset, 0);
  #endif
  }
  // count rendered polygons
  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += (pSkyDomeMesh->_GetNumInds() / 3);
  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;

  ef->FXEndPass();
  ef->FXEnd();

  if (m_pStars)
	  m_pStars->Render();

  if(bPrevClipPl)
  {
	  rd->FX_SetClipPlane(true, &rd->m_RP.m_CurClipPlane.m_Normal.x, rd->m_RP.m_bClipPlaneRefract);
  }

  rd->FX_PopVP();

  gcpRendD3D->FX_ResetPipe(); 

	return true;
}

void CStars::Render()
{
	CD3D9Renderer* rd(gcpRendD3D);
	
	I3DEngine* p3DEngine(gEnv->p3DEngine);
	float starIntensity(p3DEngine->GetGlobalParameter(E3DPARAM_NIGHSKY_STAR_INTENSITY));

	//static int s_r_stars(1);
	//static ICVar* s_pCVar_r_stars(0);
	//if (!s_pCVar_r_stars)
	//	s_pCVar_r_stars = REGISTER_CVAR2("r_stars", &s_r_stars, s_r_stars,VF_NULL,"");

	//if (!s_r_stars)
	//	return;

	if (/*m_pStarVB*/m_pStarMesh && m_pShader && rd->m_RP.m_nPassGroupID == EFSLIST_GENERAL && (rd->m_RP.m_nBatchFilter & FB_GENERAL) && starIntensity > 1e-3f)
	{
		//////////////////////////////////////////////////////////////////////////
		// set shader

		m_pShader->FXSetTechnique(m_shaderTech);
		uint32 nPasses(0);
		m_pShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
		if (!nPasses)
			return;
		m_pShader->FXBeginPass(0);
		
		//////////////////////////////////////////////////////////////////////////
		// setup params

		int vpX(0), vpY(0), vpWidth(0), vpHeight(0);
		rd->GetViewport(&vpX, &vpY, &vpWidth, &vpHeight);
		float size((float)max(3, (int) (8.0f * min(vpWidth, vpHeight) / 768.0f)));
    float flickerTime(gEnv->pTimer->GetCurrTime());
#if defined (DIRECT3D9) || defined(OPENGL) || defined(PS3)
		Vec4 paramStarSize(size, 0, 0, flickerTime * 0.5f);
#elif defined (DIRECT3D10)
		Vec4 paramStarSize(size / (float) vpWidth, size / (float) vpHeight, 0, flickerTime * 0.5f);
#endif
		m_pShader->FXSetVSFloat(m_vspnStarSize, &paramStarSize, 1); 

		Vec4 paramStarIntensity(starIntensity, 0, 0, 0);
		m_pShader->FXSetPSFloat(m_pspnStarIntensity, &paramStarIntensity, 1);

		//////////////////////////////////////////////////////////////////////////
		// commit & draw

    int32 nRenderState = GS_BLSRC_ONE | GS_BLDST_ONE;










    rd->EF_SetState( nRenderState );

		rd->FX_Commit();
		if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3S_C4B_T2S)))
    {
		  int offset(0);
		  //void* pVB(m_pStarVB->GetStream(VSF_GENERAL, &offset));
		  //rd->FX_SetVStream(0, pVB, offset, m_VertexSize[m_pStarVB->m_vertexformat]);
		  CRenderMesh2* pStarMesh((CRenderMesh2*) m_pStarMesh);
		  pStarMesh->CheckUpdate(pStarMesh->_GetVertexFormat(), 0);
      D3DVertexBuffer *pVB = rd->m_DevBufMan.GetD3DVB(pStarMesh->_GetVBStream(VSF_GENERAL), &offset);
		  rd->FX_SetVStream(0, pVB, offset, pStarMesh->GetStreamStride(VSF_GENERAL));
		  rd->FX_SetIStream(0);

  #if defined (DIRECT3D9) || defined(OPENGL)
		  rd->m_pd3dDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE); // TODO: encapsulate state in renderer!
		  rd->m_pd3dDevice->DrawPrimitive(D3DPT_POINTLIST, 0, m_numStars);
		  rd->m_pd3dDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
  #elif defined (DIRECT3D10)




		  rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		  rd->m_pd3dDeviceContext->Draw(m_numStars, 0);



  #endif

		  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += m_numStars * 2;
		  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
    }

		m_pShader->FXEndPass();
		m_pShader->FXEnd();
	}
}

bool CREFogVolume::mfDraw( CShader* ef, SShaderPass* sfm )
{
	CD3D9Renderer* rd( gcpRendD3D );

	PROFILE_LABEL_PUSH( "FOG_VOLUME" );

	// render
	uint32 nPasses( 0 );
	ef->FXBegin( &nPasses, 0 );
	if( 0 == nPasses)
	{
		assert(0);
		return( false );
	}
	ef->FXBeginPass( 0 );

	if( false != m_viewerInsideVolume )
	{
		rd->SetCullMode( R_CULL_FRONT );
		rd->EF_SetState( GS_COLMASK_RGB | GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA );
	}
	else
	{
		rd->SetCullMode( R_CULL_BACK );
		rd->EF_SetState( GS_COLMASK_RGB | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA );
	}	

	// set vs constants
	static CCryName invObjSpaceMatrixName("invObjSpaceMatrix");
	ef->FXSetVSFloat( invObjSpaceMatrixName, (const Vec4*) &m_matWSInv.m00, 3 );

	const Vec4 cEyePosVec( m_eyePosInWS, !m_viewerInsideVolume ? 1 : 0);
	static CCryName eyePosInWSName("eyePosInWS");
	ef->FXSetVSFloat( eyePosInWSName, &cEyePosVec, 1 );

	const Vec4 cEyePosInOSVec( m_eyePosInOS, 0 );
	static CCryName eyePosInOSName("eyePosInOS");
	ef->FXSetVSFloat( eyePosInOSName, &cEyePosInOSVec, 1 );

	// set ps constants
	const Vec4 cFogColVec( m_fogColor.r, m_fogColor.g, m_fogColor.b, 0 );
	static CCryName fogColorName("fogColor");
	ef->FXSetPSFloat( fogColorName, &cFogColVec, 1 );

	const Vec4 cGlobalDensityVec( m_globalDensity, 1.44269502f * m_globalDensity, 0, 0 );
	static CCryName globalDensityName("globalDensity");
	ef->FXSetPSFloat( globalDensityName, &cGlobalDensityVec, 1 );

	const Vec4 cHeigthFallOffBasePointVec( m_heightFallOffBasePoint, 0 );
	static CCryName heightFallOffBasePointName("heightFallOffBasePoint");
	ef->FXSetPSFloat( heightFallOffBasePointName, &cHeigthFallOffBasePointVec, 1 );

	const Vec4 cHeightFallOffDirScaledVec( m_heightFallOffDirScaled, 0 );
	static CCryName heightFallOffDirScaledName("heightFallOffDirScaled");
	ef->FXSetPSFloat( heightFallOffDirScaledName, &cHeightFallOffDirScaledVec, 1 );

	const Vec4 cOutsideSoftEdgesLerpVec( m_softEdgesLerp.x, m_softEdgesLerp.y, 0, 0 );
	static CCryName outsideSoftEdgesLerpName("outsideSoftEdgesLerp");
	ef->FXSetPSFloat( outsideSoftEdgesLerpName, &cOutsideSoftEdgesLerpVec, 1 );

	const Vec4 cEyePosInWSVec( m_eyePosInWS, 0 );
	ef->FXSetPSFloat( eyePosInWSName, &cEyePosInWSVec, 1 );

	const Vec4 cEyePosInOSx2Vec( 2.0f * m_eyePosInOS, 0 );
	static CCryName eyePosInOSx2Name("eyePosInOSx2");
	ef->FXSetPSFloat( eyePosInOSx2Name, &cEyePosInOSx2Vec, 1 );

	// commit all render changes
	rd->FX_Commit();

	// set vertex declaration and streams of skydome
	if (!FAILED(rd->FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
  {
	  // define bounding box geometry
	  const uint32 c_numBBVertices( 8 );	
	  SVF_P3F_C4B_T2F bbVertices[ c_numBBVertices ] =
	  {
		  { Vec3( m_localAABB.min.x, m_localAABB.min.y, m_localAABB.min.z ) }, 
		  { Vec3( m_localAABB.min.x, m_localAABB.max.y, m_localAABB.min.z ) }, 
		  { Vec3( m_localAABB.max.x, m_localAABB.max.y, m_localAABB.min.z ) }, 
		  { Vec3( m_localAABB.max.x, m_localAABB.min.y, m_localAABB.min.z ) }, 
		  { Vec3( m_localAABB.min.x, m_localAABB.min.y, m_localAABB.max.z ) }, 
		  { Vec3( m_localAABB.min.x, m_localAABB.max.y, m_localAABB.max.z ) }, 
		  { Vec3( m_localAABB.max.x, m_localAABB.max.y, m_localAABB.max.z ) }, 
		  { Vec3( m_localAABB.max.x, m_localAABB.min.y, m_localAABB.max.z ) }
	  };

	  const uint32 c_numBBIndices( 36 );
	  static const uint16 bbIndices[ c_numBBIndices ] =
	  {
		  0, 1, 2,   0, 2, 3,
		  7, 6, 5,   7, 5, 4,
		  3, 2, 6,   3, 6, 7,
		  4, 5, 1,   4, 1, 0,
		  1, 5, 6,   1, 6, 2,
		  4, 0, 3,   4, 3, 7
	  };	

	  // copy vertices into dynamic VB
	  int nVBOffs;
	  SVF_P3F_C4B_T2F* pVB( (SVF_P3F_C4B_T2F*) rd->GetVBPtr( c_numBBVertices, nVBOffs, POOL_P3F_COL4UB_TEX2F ) );
	  memcpy( pVB, bbVertices, c_numBBVertices * sizeof( SVF_P3F_C4B_T2F ) );
	  rd->UnlockVB( POOL_P3F_COL4UB_TEX2F );

	  // copy indices into dynamic IB
	  int nIBOffs;
	  uint16* pIB( rd->GetIBPtr( c_numBBIndices, nIBOffs ) );
	  memcpy( pIB, bbIndices, c_numBBIndices * sizeof( uint16 ) );
	  rd->UnlockIB();

	  // set streams
	  HRESULT hr( S_OK );
	  hr = rd->FX_SetVStream( 0, rd->m_pVB[ POOL_P3F_COL4UB_TEX2F ], 0, sizeof( SVF_P3F_C4B_T2F ) );
	  hr = rd->FX_SetIStream( rd->m_pIB );

	  // draw skydome
  #if defined (DIRECT3D9) || defined(OPENGL)
	  hr = rd->m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, nVBOffs, 0, c_numBBVertices, nIBOffs, c_numBBIndices / 3 );
  #elif defined (DIRECT3D10)
	  rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	  rd->m_pd3dDeviceContext->DrawIndexed(c_numBBIndices, nIBOffs, nVBOffs);
  #endif

	  // count rendered polygons
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += (c_numBBIndices / 3);
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
  }

	ef->FXEndPass();
	ef->FXEnd();

	PROFILE_LABEL_POP( "FOG_VOLUME" );

	return( true );
}


bool CREVolumeObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
	CD3D9Renderer* rd(gcpRendD3D);
	I3DEngine* p3DEngine(gEnv->p3DEngine);

  uint32 nFlagsPS2 = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2;
	rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);

	// render
	uint32 nPasses(0);
	ef->FXBegin(&nPasses, 0);
	if (!nPasses)
		return false;

	ef->FXBeginPass(0);

	if (m_nearPlaneIntersectsVolume)
	{
		rd->SetCullMode(R_CULL_FRONT);
		rd->EF_SetState(GS_COLMASK_RGB | GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
	}
	else
	{
		rd->SetCullMode(R_CULL_BACK );
		rd->EF_SetState(GS_COLMASK_RGB | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
	}	

	// set vs constants
	static CCryName invObjSpaceMatrixName("invObjSpaceMatrix");
	ef->FXSetVSFloat(invObjSpaceMatrixName, (const Vec4*) &m_matInv.m00, 3);

	const Vec4 cEyePosVec(m_eyePosInWS, 0);
	static CCryName eyePosInWSName("eyePosInWS");
	ef->FXSetVSFloat(eyePosInWSName, &cEyePosVec, 1);

	const Vec4 cViewerOutsideVec(!m_viewerInsideVolume ? 1 : 0, m_nearPlaneIntersectsVolume ? 1 : 0, 0, 0);
	static CCryName viewerIsOutsideName("viewerIsOutside");
	ef->FXSetVSFloat(viewerIsOutsideName, &cViewerOutsideVec, 1);

	const Vec4 cEyePosInOSVec(m_eyePosInOS, 0);
	static CCryName eyePosInOSName("eyePosInOS");
	ef->FXSetVSFloat(eyePosInOSName, &cEyePosInOSVec, 1);

	// set ps constants
	const Vec4 cEyePosInWSVec(m_eyePosInWS, 0);
	ef->FXSetPSFloat(eyePosInWSName, &cEyePosInWSVec, 1);
	
	ColorF specColor(1,1,1,1), diffColor(1,1,1,1);
	SRenderShaderResources* pRes(rd->m_RP.m_pShaderResources);
	if (pRes && pRes->m_Constants[eHWSC_Pixel].size())
	{
		ColorF* pSrc = (ColorF*) &pRes->m_Constants[eHWSC_Pixel][0];
		specColor = pSrc[PS_SPECULAR_COL];
		diffColor = pSrc[PS_DIFFUSE_COL];
	}

	float sunColorMul(0), skyColorMul(0);
	p3DEngine->GetCloudShadingMultiplier(sunColorMul, skyColorMul);

	Vec3 brightColor(p3DEngine->GetSunColor() * sunColorMul);
	brightColor = brightColor.CompMul(Vec3(specColor.r, specColor.g, specColor.b));

	Vec3 darkColor(p3DEngine->GetSkyColor() * skyColorMul);
	darkColor = darkColor.CompMul(Vec3(diffColor.r, diffColor.g, diffColor.b));

	{
		static CCryName darkColorName("darkColor");
		const Vec4 data(darkColor, m_alpha);
		ef->FXSetPSFloat(darkColorName, &data, 1);
	}
	{
		static CCryName brightColorName("brightColor");
		const Vec4 data(brightColor, m_alpha);
		ef->FXSetPSFloat(brightColorName, &data, 1);
	}

	const Vec4 cVolumeTraceStartPlane(m_volumeTraceStartPlane.n, m_volumeTraceStartPlane.d);
	static CCryName volumeTraceStartPlaneName("volumeTraceStartPlane");
	ef->FXSetPSFloat(volumeTraceStartPlaneName, &cVolumeTraceStartPlane, 1);

	const Vec4 cScaleConsts(m_scale, 0, 0, 0);
	static CCryName scaleConstsName("scaleConsts");
	ef->FXSetPSFloat(scaleConstsName, &cScaleConsts, 1);

	// TODO: optimize shader and remove need to pass inv obj space matrix
	ef->FXSetPSFloat(invObjSpaceMatrixName, (const Vec4*) &m_matInv.m00, 3);


	// commit all render changes
	rd->FX_Commit();

	// set vertex declaration and streams
	if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
	{
//		static ICVar* s_pCVar = 0;
//		static int s_test = 0;
//
//		if (!s_pCVar)
//			s_pCVar = gEnv->pConsole->Register("r_volobj_new", &s_test, 0);
//
//		if (!s_test)
//		{
//		// define bounding box geometry
//		const uint32 c_numBBVertices(8);
//		SVF_P3F_C4B_T2F bbVertices[c_numBBVertices] =
//		{
//			{Vec3(m_renderBoundsOS.min.x, m_renderBoundsOS.min.y, m_renderBoundsOS.min.z)}, 
//			{Vec3(m_renderBoundsOS.min.x, m_renderBoundsOS.max.y, m_renderBoundsOS.min.z)}, 
//			{Vec3(m_renderBoundsOS.max.x, m_renderBoundsOS.max.y, m_renderBoundsOS.min.z)}, 
//			{Vec3(m_renderBoundsOS.max.x, m_renderBoundsOS.min.y, m_renderBoundsOS.min.z)}, 
//			{Vec3(m_renderBoundsOS.min.x, m_renderBoundsOS.min.y, m_renderBoundsOS.max.z)}, 
//			{Vec3(m_renderBoundsOS.min.x, m_renderBoundsOS.max.y, m_renderBoundsOS.max.z)}, 
//			{Vec3(m_renderBoundsOS.max.x, m_renderBoundsOS.max.y, m_renderBoundsOS.max.z)}, 
//			{Vec3(m_renderBoundsOS.max.x, m_renderBoundsOS.min.y, m_renderBoundsOS.max.z)}
//		};
//
//		const uint32 c_numBBIndices(36);
//		static const uint16 bbIndices[c_numBBIndices] =
//		{
//			0, 1, 2,   0, 2, 3,
//			7, 6, 5,   7, 5, 4,
//			3, 2, 6,   3, 6, 7,
//			4, 5, 1,   4, 1, 0,
//			1, 5, 6,   1, 6, 2,
//			4, 0, 3,   4, 3, 7
//		};	
//
//		// copy vertices into dynamic VB
//		int nVBOffs;
//		SVF_P3F_C4B_T2F* pVB((SVF_P3F_C4B_T2F*) rd->GetVBPtr(c_numBBVertices, nVBOffs, POOL_P3F_COL4UB_TEX2F));
//		memcpy(pVB, bbVertices, c_numBBVertices * sizeof( SVF_P3F_C4B_T2F));
//		rd->UnlockVB(POOL_P3F_COL4UB_TEX2F);
//
//		// copy indices into dynamic IB
//		int nIBOffs;
//		uint16* pIB(rd->GetIBPtr(c_numBBIndices, nIBOffs));
//		memcpy(pIB, bbIndices, c_numBBIndices * sizeof(uint16));
//		rd->UnlockIB();
//
//		// set streams
//		HRESULT hr(S_OK);
//		hr = rd->FX_SetVStream(0, rd->m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
//		hr = rd->FX_SetIStream(rd->m_pIB);
//
//		// draw
//#if defined (DIRECT3D9) || defined(OPENGL)
//		hr = rd->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, nVBOffs, 0, c_numBBVertices, nIBOffs, c_numBBIndices / 3);
//#elif defined (DIRECT3D10)
//		rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//		rd->m_pd3dDeviceContext->DrawIndexed(c_numBBIndices, nIBOffs, nVBOffs);
//#endif
//
//		// count rendered polygons
//		rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += (c_numBBIndices / 3);
//		rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
//		}
//		else
//		{
		CRenderMesh2* pHullMesh = (CRenderMesh2*) m_pHullMesh;

		// set vertex and index buffer
		pHullMesh->CheckUpdate(pHullMesh->_GetVertexFormat(), 0);
    int vbOffset(0);
    int ibOffset(0);
    D3DVertexBuffer *pVB = rd->m_DevBufMan.GetD3DVB(pHullMesh->_GetVBStream(VSF_GENERAL), &vbOffset);
    D3DIndexBuffer *pIB = rd->m_DevBufMan.GetD3DIB(pHullMesh->_GetIBStream(), &ibOffset);
    assert(pVB);
    assert(pIB);
    if(!pVB || !pIB)
      return false;

		HRESULT hr(S_OK);
		hr = rd->FX_SetVStream(0, pVB, vbOffset, pHullMesh->GetStreamStride(VSF_GENERAL));

		ibOffset /= sizeof(uint16);
		hr = rd->FX_SetIStream(pIB);

		// draw
#if defined (DIRECT3D9) || defined(OPENGL)
		hr = rd->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pHullMesh->_GetNumVerts(), ibOffset, pHullMesh->_GetNumInds() / 3);
#elif defined (DIRECT3D10)
		rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		rd->m_pd3dDeviceContext->DrawIndexed(pHullMesh->_GetNumInds(), ibOffset, 0);
#endif

		// count rendered polygons
		rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += (pHullMesh->_GetNumInds() / 3);
		rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
//		}
	}

	ef->FXEndPass();
	ef->FXEnd();

	rd->FX_ResetPipe();
  rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 = nFlagsPS2;

	return true;
}


#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
bool CREPrismObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
	CD3D9Renderer* rd(gcpRendD3D);

	// render
	uint32 nPasses(0);
	ef->FXBegin(&nPasses, 0);
	if (!nPasses)
		return false;

	ef->FXBeginPass(0);

	// commit all render changes
//	rd->FX_Commit();

	static SVF_P3F_C4B_T2F pScreenQuad[] =
	{
	{ Vec3(0, 0, 0), 0, Vec2(0, 0) },   
	{ Vec3(0, 1, 0), 0, Vec2(0, 1) },    
	{ Vec3(1, 0, 0), 0, Vec2(1, 0) },   
	{ Vec3(1, 1, 0), 0, Vec2(1, 1) },   
	};

	pScreenQuad[0].xyz = Vec3(0, 0, 0);
	pScreenQuad[1].xyz = Vec3(0, 1, 0);
	pScreenQuad[2].xyz = Vec3(1, 0, 0);
	pScreenQuad[3].xyz = Vec3(1, 1, 0);

	CVertexBuffer strip(pScreenQuad, eVF_P3F_C4B_T2F);
	gcpRendD3D->DrawPrimitives(&strip, 4);   

	// count rendered polygons
//	rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += 2;
//	rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;

	ef->FXEndPass();
	ef->FXEnd();

	return true;
}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE


bool CREWaterVolume::mfDraw( CShader* ef, SShaderPass* sfm )
{
	assert( m_pParams );
	if( !m_pParams )
		return false;

	CD3D9Renderer* rd( gcpRendD3D );

	if( !m_drawWaterSurface && m_pParams->m_viewerInsideVolume )
	{
		// set projection matrix for full screen quad
		rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Push();
		D3DXMATRIXA16 *m = (D3DXMATRIXA16*)rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->GetTop();
		mathMatrixOrthoOffCenterLH((Matrix44A*)m, -1, 1, -1, 1, -1, 1);
		if (SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID] <= 1)
		{
			const CD3D9Renderer::SRenderTileInfo& rti = rd->GetRenderTileInfo();
			if (rti.nGridSizeX>1.f || rti.nGridSizeY>1.f)
			{ // shift and scale viewport
				(*m)._11 *= rti.nGridSizeX;
				(*m)._22 *= rti.nGridSizeY;
				(*m)._41 = -((rti.nGridSizeX-1.f)-rti.nPosX*2.0f);
				(*m)._42 =  ((rti.nGridSizeY-1.f)-rti.nPosY*2.0f);
			}
		}

		// set view matrix to identity
		rd->PushMatrix();
		rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

		rd->EF_DirtyMatrix();
	}

	// render
	uint32 nPasses( 0 );
	ef->FXBegin( &nPasses, 0 );
	if( 0 == nPasses)
		return false;
	ef->FXBeginPass( 0 );

	// set ps constants
	if( !m_drawWaterSurface )
	{
		if( !m_pOceanParams )
		{
			// fog color & density
			Vec4 fogColorDensity( m_pParams->m_fogColor.CompMul( gEnv->p3DEngine->GetSunColor() ), 1.44269502f * m_pParams->m_fogDensity ); // log2(e) = 1.44269502
      static CCryName Param1Name = "cFogColorDensity";
			ef->FXSetPSFloat(Param1Name, &fogColorDensity, 1 );
		}
		else
		{
			// fog color & density
			Vec4 fogColorDensity( m_pOceanParams->m_fogColor.CompMul( gEnv->p3DEngine->GetSunColor() ), 1.44269502f * m_pOceanParams->m_fogDensity ); // log2(e) = 1.44269502
      static CCryName Param1Name = "cFogColorDensity";
			ef->FXSetPSFloat(Param1Name, &fogColorDensity, 1 );

			// fog color shallow & water level
			Vec4 fogColorShallowWaterLevel( m_pOceanParams->m_fogColorShallow.CompMul( gEnv->p3DEngine->GetSunColor() ), -m_pParams->m_fogPlane.d );
      static CCryName Param2Name = "cFogColorShallowWaterLevel";
			ef->FXSetPSFloat(Param2Name, &fogColorShallowWaterLevel, 1 );

			if( m_pParams->m_viewerInsideVolume )
			{
				// under water in-scattering constant term = exp2( -fogDensity * ( waterLevel - cameraPos.z) )
				float c( expf( -m_pOceanParams->m_fogDensity * ( -m_pParams->m_fogPlane.d - rd->GetCamera().GetPosition().z ) ) );
				Vec4 underWaterInScatterConst( c, 0, 0, 0 );
        static CCryName Param3Name = "cUnderWaterInScatterConst";
				ef->FXSetPSFloat(Param3Name, &underWaterInScatterConst, 1 );
			}
		}

		// fog plane
		Vec4 fogPlane( m_pParams->m_fogPlane.n.x, m_pParams->m_fogPlane.n.y, m_pParams->m_fogPlane.n.z, m_pParams->m_fogPlane.d );
    static CCryName Param4Name = "cFogPlane";
		ef->FXSetPSFloat(Param4Name, &fogPlane, 1 );

		if( m_pParams->m_viewerInsideVolume )
		{
			Vec4 perpDist( m_pParams->m_fogPlane | rd->GetRCamera().Orig, 0, 0, 0 );
      static CCryName Param5Name = "cPerpDist";
			ef->FXSetPSFloat(Param5Name, &perpDist, 1 );
		}
	}

	// set vs constants
	Vec4 viewerColorToWaterPlane( m_pParams->m_viewerCloseToWaterPlane ? 0.0f : 1.0f, 0.0f, 0.0f, 0.0f );
  static CCryName Param6Name = "cViewerColorToWaterPlane";
	ef->FXSetVSFloat(Param6Name, &viewerColorToWaterPlane, 1 );

	if( m_drawWaterSurface || !m_pParams->m_viewerInsideVolume )
	{
		// copy vertices into dynamic VB
		int nVBOffs;
		SVF_P3F_C4B_T2F* pVB( (SVF_P3F_C4B_T2F*) rd->GetVBPtr( m_pParams->m_numVertices, nVBOffs, POOL_P3F_COL4UB_TEX2F ) );
		memcpy( pVB, m_pParams->m_pVertices, m_pParams->m_numVertices * sizeof( SVF_P3F_C4B_T2F ) );
		rd->UnlockVB( POOL_P3F_COL4UB_TEX2F );

		// copy indices into dynamic IB
		int nIBOffs;
		uint16* pIB( rd->GetIBPtr( m_pParams->m_numIndices, nIBOffs ) );
		memcpy( pIB, m_pParams->m_pIndices, m_pParams->m_numIndices * sizeof( uint16 ) );
		rd->UnlockIB();

		// set streams
		HRESULT hr( S_OK );
		hr = rd->FX_SetVStream( 0, rd->m_pVB[ POOL_P3F_COL4UB_TEX2F ], 0, sizeof( SVF_P3F_C4B_T2F ) );
		hr = rd->FX_SetIStream( rd->m_pIB );

		// set vertex declaration
		if (!FAILED(rd->FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
    {
		  // commit all render changes
		  rd->FX_Commit();

		  // draw
  #if defined (DIRECT3D9)
		  hr = rd->m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, nVBOffs, 0, m_pParams->m_numVertices, nIBOffs, m_pParams->m_numIndices / 3 );
  #elif defined (DIRECT3D10)    
      rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      rd->m_pd3dDeviceContext->DrawIndexed(m_pParams->m_numIndices, nIBOffs, nVBOffs);
  #endif

		  // count rendered polygons
		  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += (m_pParams->m_numIndices / 3);
		  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
    }
	}
	else
	{
		// copy vertices into dynamic VB
		int nVBOffs;

		SVF_P3F_T3F* pVB( (SVF_P3F_T3F*) rd->GetVBPtr( 4, nVBOffs, POOL_P3F_TEX3F ) );

		// Need to offset half a texel...
		// Since we operate in camera space where the viewport goes from (-1, -1) to (1, 1) the offset is 1/width, 
		// 1/height instead of 0.5/width, 0.5/height. Also, y is still flipped so offsetY needs to be inverted.
		int x(0), y(0), w(0), h(0);
		rd->GetViewport(&x, &y, &w, &h);
		float offsetX(-1.0f / (float)w);
		float offsetY(1.0f / (float)h);

#if defined (DIRECT3D10)
     offsetX = 0.0f;
     offsetY = 0.0f;
#endif

		Vec3 coords[8];
		rd->GetRCamera().CalcVerts( coords );

		pVB[0].p.x = -1 + offsetX;
		pVB[0].p.y = 1 + offsetY;
		pVB[0].p.z = 0.5f;
		pVB[0].st = coords[5] - coords[1];

		pVB[1].p.x = 1 + offsetX;
		pVB[1].p.y = 1 + offsetY;
		pVB[1].p.z = 0.5f;
		pVB[1].st = coords[4] - coords[0];

		pVB[2].p.x = -1 + offsetX;
		pVB[2].p.y = -1 + offsetY;
		pVB[2].p.z = 0.5f;
		pVB[2].st = coords[6] - coords[2];

		pVB[3].p.x = 1 + offsetX;
		pVB[3].p.y = -1 + offsetY;
		pVB[3].p.z = 0.5f;
		pVB[3].st = coords[7] - coords[3];

		rd->UnlockVB( POOL_P3F_TEX3F );

		// set streams
		HRESULT hr( S_OK );
		hr = rd->FX_SetVStream( 0, rd->m_pVB[ POOL_P3F_TEX3F ], 0, sizeof( SVF_P3F_T3F ) );

		// set vertex declaration
		if (!FAILED(rd->FX_SetVertexDeclaration( 0, eVF_P3F_T3F )))
    {
		  // commit all render changes
		  rd->FX_Commit();

		  // draw
  #if defined (DIRECT3D9) || defined(OPENGL)
		  hr = rd->m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, nVBOffs, 2 );
  #elif defined (DIRECT3D10)
      rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      rd->m_pd3dDeviceContext->Draw(4, nVBOffs);
  #endif

		  // count rendered polygons
		  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += 2;
		  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
    }

		// reset matrices
		rd->PopMatrix();
		rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Pop();
		rd->EF_DirtyMatrix();
	}

	ef->FXEndPass();
	ef->FXEnd();

	return true;
}

bool CREWaterWave::mfDraw( CShader* ef, SShaderPass* sfm )
{
  assert( m_pParams );
  if( !m_pParams )
    return false;

  CD3D9Renderer* rd( gcpRendD3D );

  // render
  uint32 nPasses( 0 );
  ef->FXBegin( &nPasses, 0 );
  if( 0 == nPasses)
    return false;
  ef->FXBeginPass( 0 );

  // commit all render changes
  rd->FX_Commit();

  // set vertex declaration and streams of sky dome
  if (!FAILED(rd->FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
  {
    // copy vertices into dynamic VB
    int nVBOffs;
    SVF_P3F_C4B_T2F* pVB( (SVF_P3F_C4B_T2F*) rd->GetVBPtr( m_pParams->m_nVertices, nVBOffs, POOL_P3F_COL4UB_TEX2F ) );
    memcpy( pVB, m_pParams->m_pVertices, m_pParams->m_nVertices * sizeof( SVF_P3F_C4B_T2F ) );
    rd->UnlockVB( POOL_P3F_COL4UB_TEX2F );

    // copy indices into dynamic IB
    int nIBOffs;
    uint16* pIB( rd->GetIBPtr( m_pParams->m_nIndices, nIBOffs ) );
    memcpy( pIB, m_pParams->m_pIndices, m_pParams->m_nIndices * sizeof( uint16 ) );
    rd->UnlockIB();

    // set streams
    HRESULT hr( S_OK );
    hr = rd->FX_SetVStream( 0, rd->m_pVB[ POOL_P3F_COL4UB_TEX2F ], 0, sizeof( SVF_P3F_C4B_T2F ) );
    hr = rd->FX_SetIStream( rd->m_pIB );

    // draw
#if defined (DIRECT3D9)
    hr = rd->m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, nVBOffs, 0, m_pParams->m_nVertices, nIBOffs, m_pParams->m_nIndices / 3 );
#elif defined (DIRECT3D10)
    assert(0);
#endif

    // count rendered polygons
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += m_pParams->m_nIndices / 3;
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
  }

  ef->FXEndPass();
  ef->FXEnd();
  
  return true;
}

void CREWaterOcean::Create( uint32 nVerticesCount, SVF_P3F_C4B_T2F *pVertices, uint32 nIndicesCount, uint16 *pIndices)
{
  if( !nVerticesCount || !pVertices || !nIndicesCount || !pIndices)
    return;

  CD3D9Renderer* rd( gcpRendD3D );
  ReleaseOcean();

  m_nVerticesCount = nVerticesCount;
  m_nIndicesCount = nIndicesCount;
  HRESULT hr( S_OK );
#if defined (DIRECT3D9) || defined(OPENGL)

  LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Create vertex buffer
  //////////////////////////////////////////////////////////////////////////////////////////////////

  {
    SVF_P3F_C4B_T2F *dst;
    D3DVertexBuffer* ibuf;

    uint32 size = nVerticesCount * sizeof(SVF_P3F_C4B_T2F);
    hr = dv->CreateVertexBuffer(size , D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, (D3DVertexBuffer**)&m_pVertices, NULL);     
    ibuf = (D3DVertexBuffer*)m_pVertices; 

    hr = ibuf->Lock(0, 0, (void **) &dst, 0);
    //cryMemcpy(dst, pVertices, nVerticesCount * sizeof(struct_VERTEX_FORMAT_P3F)); 
    memcpy(dst, pVertices, nVerticesCount * sizeof(SVF_P3F_C4B_T2F)); 
    
    hr = ibuf->Unlock();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Create index buffer
  //////////////////////////////////////////////////////////////////////////////////////////////////


  {
    uint32 size = nIndicesCount * sizeof(uint16);
    int flags = 0;
    uint16 *dst;
    D3DIndexBuffer* ibuf;
    hr = dv->CreateIndexBuffer( size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, (D3DIndexBuffer**)&m_pIndices, NULL);
    ibuf = (D3DIndexBuffer*)m_pIndices;  

    hr = ibuf->Lock(0, 0, (void **) &dst, 0);    
    //cryMemcpy(dst, pIndices, size);
    memcpy(dst, pIndices, size);
    hr = ibuf->Unlock();
  }

#elif defined (DIRECT3D10)
  
  D3DDevice *dv = gcpRendD3D->GetD3DDevice();
  HRESULT h;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Create vertex buffer
  //////////////////////////////////////////////////////////////////////////////////////////////////
  {
    D3DVertexBuffer *pVertexBuffer = 0;
    D3D11_BUFFER_DESC BufDesc;
    SVF_P3F_C4B_T2F *dst = 0;

    uint32 size = nVerticesCount * sizeof(SVF_P3F_C4B_T2F);
    BufDesc.ByteWidth = size;
    BufDesc.Usage = D3D11_USAGE_DEFAULT;
    BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufDesc.CPUAccessFlags = 0;
    BufDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA pInitData;
    pInitData.pSysMem = pVertices;
    pInitData.SysMemPitch = 0;
    pInitData.SysMemSlicePitch = 0;

    h = dv->CreateBuffer(&BufDesc, &pInitData, &pVertexBuffer);
/*    if (SUCCEEDED(h))
    {
      byte *pData = (byte*) 0x12345678;
      h = pVertexBuffer->Map(D3D11_MAP_WRITE_DISCARD, 0, (void **)&dst);
      cryMemcpy(dst, pVertices, size);
      pVertexBuffer->Unmap();
    }*/

    m_pVertices = pVertexBuffer;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Create index buffer
  //////////////////////////////////////////////////////////////////////////////////////////////////
  {
    D3DIndexBuffer *pIndexBuffer = 0;
    uint32 size = nIndicesCount * sizeof(uint16);

    D3D11_BUFFER_DESC BufDesc;
    BufDesc.ByteWidth = size;
    BufDesc.Usage = D3D11_USAGE_DEFAULT;
    BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    BufDesc.CPUAccessFlags = 0;
    BufDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA pInitData;
    pInitData.pSysMem = pIndices;
    pInitData.SysMemPitch = 0;
    pInitData.SysMemSlicePitch = 0;

    h = dv->CreateBuffer(&BufDesc, &pInitData, &pIndexBuffer);
/*    if (SUCCEEDED(h))
    {
      h = pIndexBuffer->Map(D3D11_MAP_WRITE_DISCARD, 0, (void **)&dst);
      memcpy(dst, pIndices, size);
      pIndexBuffer->Unmap();
    }
*/
    m_pIndices = pIndexBuffer;
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::FrameUpdate()
{
  // Update Water simulator
  //  UpdateFFT();

  static bool bInitialize = true;

  static Vec4 pParams0(0, 0, 0, 0), pParams1(0, 0, 0, 0);

  Vec4 pCurrParams0, pCurrParams1;
  gEnv->p3DEngine->GetOceanAnimationParams(pCurrParams0, pCurrParams1);

  // why no comparison operator on Vec4 ??
  if( !m_pWaterSim || bInitialize || pCurrParams0.x != pParams0.x || pCurrParams0.y != pParams0.y ||
    pCurrParams0.z != pParams0.z || pCurrParams0.w != pParams0.w || pCurrParams1.x != pParams1.x || 
    pCurrParams1.y != pParams1.y || pCurrParams1.z != pParams1.z || pCurrParams1.w != pParams1.w )
  {
    pParams0 = pCurrParams0;
    pParams1 = pCurrParams1;

    SAFE_DELETE( m_pWaterSim );
    m_pWaterSim = new CWater;

    m_pWaterSim->Create( 1.0, pParams0.x, pParams0.z, 1.0f, 1.0f);

    bInitialize = false;
  }

  assert( m_pWaterSim );
  const int nGridSize = 64;

  // Update Vertex Texture
  if ( !CTexture::IsTextureExist(CTexture::s_ptexWaterOcean))
  {
    CTexture::s_ptexWaterOcean->Create2DTexture(nGridSize, nGridSize, 1, 
      FT_DONT_RELEASE | FT_NOMIPS |  FT_USAGE_DYNAMIC, 
      0, eTF_A32B32G32R32F, eTF_A32B32G32R32F);
    CTexture::s_ptexWaterOcean->Fill(ColorF(0, 0, 0, 0));
		CTexture::s_ptexWaterOcean->SetVertexTexture(true);
  }

   CTexture *pTexture = CTexture::s_ptexWaterOcean;

  // Copy data..
  if (CTexture::IsTextureExist(pTexture))
  {
		const float fUpdateTime = 0.75f * 0.125*gEnv->pTimer->GetCurrTime() / clamp_tpl<float>(pParams1.x, 0.55f, 1.0f);
		int nFrameID = gRenDev->GetFrameID();













		
    m_pWaterSim->Update( nFrameID, fUpdateTime );

    Vec4 *pDispGrid = m_pWaterSim->GetDisplaceGrid();

    uint32 pitch = 4 * sizeof( f32 )*nGridSize; 
    uint32 width = nGridSize; 
    uint32 height = nGridSize;

    STALL_PROFILER("update subresource")
    CDeviceTexture* pDevTex = pTexture->GetDevTexture();
    assert(pDevTex);
    STexLock rect;












    if (SUCCEEDED(pDevTex->LockRect(0, rect, LF_DISCARD))) 
    {
      cryMemcpy(rect.pData, pDispGrid, 4 * width * height* sizeof(f32) );
      pDevTex->UnlockRect(0);
    }




  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::ReleaseOcean()
{
#if defined (DIRECT3D9) || defined(OPENGL)

  IDirect3DVertexBuffer9 *pVertices = (IDirect3DVertexBuffer9*)m_pVertices; 
  IDirect3DIndexBuffer9 *pIndices = (IDirect3DIndexBuffer9*)m_pIndices; 

#elif defined (DIRECT3D10)

  ID3D11Buffer *pVertices = (ID3D11Buffer*)m_pVertices; 
  ID3D11Buffer *pIndices = (ID3D11Buffer*)m_pIndices; 

#endif

  SAFE_RELEASE( pVertices );
  SAFE_RELEASE( pIndices );
  SAFE_DELETE( m_pWaterSim );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREWaterOcean::mfDraw(  CShader* ef, SShaderPass* sfm  )
{
  if( !m_nVerticesCount || !m_nIndicesCount || !m_pVertices || !m_pIndices)
    return false;

  CD3D9Renderer* rd( gcpRendD3D );

  FrameUpdate();

  if( CTexture::s_ptexWaterOcean )
  {
#if defined (DIRECT3D10)
    CTexture::s_ptexWaterOcean->SetFilterMode( FILTER_LINEAR );
#else
    CTexture::s_ptexWaterOcean->SetFilterMode( FILTER_POINT );
#endif
    CTexture::s_ptexWaterOcean->SetClampingMode(0, 0, 1);
    CTexture::s_ptexWaterOcean->UpdateTexStates();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////

  uint64 nFlagsShaderRTprev = rd->m_RP.m_FlagsShader_RT;
  rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

  uint32 nFlagsPF2prev = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2;
  rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_COMMIT_PF|RBPF2_COMMIT_CM);  

  // render
  uint32 nPasses( 0 );
  ef->FXSetTechnique("WaterFFT");
  ef->FXBegin( &nPasses, 0 );

  if( 0 == nPasses)
    return false;
  ef->FXBeginPass(0);

  // set streams
  HRESULT hr( S_OK );

  STexStageInfo pPrevTexState0 = CTexture::s_TexStages[0];

  if( CTexture::s_ptexWaterOcean )
  {
#if defined (DIRECT3D10)
    STexState pState(FILTER_BILINEAR, false);
#else
    STexState pState(FILTER_POINT, false);
#endif
     
    const int texStateID(CTexture::GetTexState(pState));
    CTexture::s_ptexWaterOcean->Apply(0, texStateID);
  }
  // commit all render changes
  rd->FX_Commit();

  // draw
  hr= rd->FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F );
  if (!FAILED(hr))
  {
    hr = rd->FX_SetVStream(0, m_pVertices, 0, sizeof(SVF_P3F_C4B_T2F));
    hr= rd->FX_SetIStream(m_pIndices);
 
#if defined (DIRECT3D9)
    hr = rd->m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLESTRIP, 0, 0, m_nVerticesCount, 0, m_nIndicesCount-2 );
  #elif defined (DIRECT3D10)
    rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    rd->m_pd3dDeviceContext->DrawIndexed(m_nIndicesCount, 0, 0);
  #endif
}

  ef->FXEndPass();
  ef->FXEnd();

  CTexture::s_TexStages[0] = pPrevTexState0;

  gcpRendD3D->FX_ResetPipe(); 

  // count rendered polygons
  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += m_nIndicesCount -2;
  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
  
  rd->m_RP.m_FlagsShader_RT = nFlagsShaderRTprev;
  rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 = nFlagsPF2prev;

  return true;
}


//=========================================================================================
/*
void _Draw3dBBoxSolid(const Vec3 &mins,const Vec3 &maxs)
{
  int nOffs;
  CD3D9Renderer *r = gcpRendD3D;
  LPDIRECT3DDEVICE9 dv = r->GetD3DDevice();
  HRESULT hr;
  SVF_P3F_C4B_T2F *vQuad;

  r->EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, eCA_Diffuse|(eCA_Diffuse<<3), eCA_Diffuse|(eCA_Diffuse<<3));
  r->FX_SetFPMode();

  // Set the vertex shader to the FVF fixed function shader
  r->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);

  vQuad = (SVF_P3F_C4B_T2F *)r->GetVBPtr(4, nOffs);
  vQuad[0].xyz = Vec3(mins.x,mins.y,mins.z); vQuad[0].st[0] = vQuad[0].st[1] = 0; vQuad[0].color.dcolor = -1;
  vQuad[3].xyz = Vec3(mins.x,mins.y,maxs.z); vQuad[1].st[0] = vQuad[1].st[1] = 0; vQuad[1].color.dcolor = -1;
  vQuad[2].xyz = Vec3(maxs.x,mins.y,maxs.z); vQuad[2].st[0] = vQuad[2].st[1] = 0; vQuad[2].color.dcolor = -1;
  vQuad[1].xyz = Vec3(maxs.x,mins.y,mins.z); vQuad[3].st[0] = vQuad[3].st[1] = 0; vQuad[3].color.dcolor = -1;
  r->UnlockVB();
  // Bind our vertex as the first data stream of our device
  r->FX_SetVStream(0, r->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  // Render the two triangles from the data stream
  hr = dv->DrawPrimitive(D3DPT_TRIANGLEFAN, nOffs, 2);

  vQuad = (SVF_P3F_C4B_T2F *)r->GetVBPtr(4, nOffs);
  vQuad[0].xyz = Vec3(mins.x,mins.y,mins.z); vQuad[0].st[0] = vQuad[0].st[1] = 0; vQuad[0].color.dcolor = -1;
  vQuad[1].xyz = Vec3(mins.x,mins.y,maxs.z); vQuad[1].st[0] = vQuad[1].st[1] = 0; vQuad[1].color.dcolor = -1;
  vQuad[2].xyz = Vec3(mins.x,maxs.y,maxs.z); vQuad[2].st[0] = vQuad[2].st[1] = 0; vQuad[2].color.dcolor = -1;
  vQuad[3].xyz = Vec3(mins.x,maxs.y,mins.z); vQuad[3].st[0] = vQuad[3].st[1] = 0; vQuad[3].color.dcolor = -1;
  r->UnlockVB();
  // Bind our vertex as the first data stream of our device
  r->FX_SetVStream(0, r->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  // Render the two triangles from the data stream
  hr = dv->DrawPrimitive(D3DPT_TRIANGLEFAN, nOffs, 2);

  vQuad = (SVF_P3F_C4B_T2F *)r->GetVBPtr(4, nOffs);
  vQuad[0].xyz = Vec3(mins.x,maxs.y,mins.z); vQuad[0].st[0] = vQuad[0].st[1] = 0; vQuad[0].color.dcolor = -1;
  vQuad[1].xyz = Vec3(mins.x,maxs.y,maxs.z); vQuad[1].st[0] = vQuad[1].st[1] = 0; vQuad[1].color.dcolor = -1;
  vQuad[2].xyz = Vec3(maxs.x,maxs.y,maxs.z); vQuad[2].st[0] = vQuad[2].st[1] = 0; vQuad[2].color.dcolor = -1;
  vQuad[3].xyz = Vec3(maxs.x,maxs.y,mins.z); vQuad[3].st[0] = vQuad[3].st[1] = 0; vQuad[3].color.dcolor = -1;
  r->UnlockVB();
  // Bind our vertex as the first data stream of our device
  r->FX_SetVStream(0, r->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  // Render the two triangles from the data stream
  hr = dv->DrawPrimitive(D3DPT_TRIANGLEFAN, nOffs, 2);

  vQuad = (SVF_P3F_C4B_T2F *)r->GetVBPtr(4, nOffs);
  vQuad[0].xyz = Vec3(maxs.x,maxs.y,mins.z); vQuad[0].st[0] = vQuad[0].st[1] = 0; vQuad[0].color.dcolor = -1;
  vQuad[1].xyz = Vec3(maxs.x,maxs.y,maxs.z); vQuad[1].st[0] = vQuad[1].st[1] = 0; vQuad[1].color.dcolor = -1;
  vQuad[2].xyz = Vec3(maxs.x,mins.y,maxs.z); vQuad[2].st[0] = vQuad[2].st[1] = 0; vQuad[2].color.dcolor = -1;
  vQuad[3].xyz = Vec3(maxs.x,mins.y,mins.z); vQuad[3].st[0] = vQuad[3].st[1] = 0; vQuad[3].color.dcolor = -1;
  r->UnlockVB();
  // Bind our vertex as the first data stream of our device
  r->FX_SetVStream(0, r->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  // Render the two triangles from the data stream
  hr = dv->DrawPrimitive(D3DPT_TRIANGLEFAN, nOffs, 2);

  // top
  vQuad = (SVF_P3F_C4B_T2F *)r->GetVBPtr(4, nOffs);
  vQuad[0].xyz = Vec3(maxs.x,maxs.y,maxs.z); vQuad[0].st[0] = vQuad[0].st[1] = 0; vQuad[0].color.dcolor = -1;
  vQuad[1].xyz = Vec3(mins.x,maxs.y,maxs.z); vQuad[1].st[0] = vQuad[1].st[1] = 0; vQuad[1].color.dcolor = -1;
  vQuad[2].xyz = Vec3(mins.x,mins.y,maxs.z); vQuad[2].st[0] = vQuad[2].st[1] = 0; vQuad[2].color.dcolor = -1;
  vQuad[3].xyz = Vec3(maxs.x,mins.y,maxs.z); vQuad[3].st[0] = vQuad[3].st[1] = 0; vQuad[3].color.dcolor = -1;
  r->UnlockVB();
  // Bind our vertex as the first data stream of our device
  r->FX_SetVStream(0, r->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  // Render the two triangles from the data stream
  hr = dv->DrawPrimitive(D3DPT_TRIANGLEFAN, nOffs, 2);

  // bottom
  vQuad = (SVF_P3F_C4B_T2F *)r->GetVBPtr(4, nOffs);
  vQuad[0].xyz = Vec3(maxs.x,mins.y,mins.z); vQuad[0].st[0] = vQuad[0].st[1] = 0; vQuad[0].color.dcolor = -1;
  vQuad[1].xyz = Vec3(mins.x,mins.y,mins.z); vQuad[1].st[0] = vQuad[1].st[1] = 0; vQuad[1].color.dcolor = -1;
  vQuad[2].xyz = Vec3(mins.x,maxs.y,mins.z); vQuad[2].st[0] = vQuad[2].st[1] = 0; vQuad[2].color.dcolor = -1;
  vQuad[3].xyz = Vec3(maxs.x,maxs.y,mins.z); vQuad[3].st[0] = vQuad[3].st[1] = 0; vQuad[3].color.dcolor = -1;
  r->UnlockVB();
  // Bind our vertex as the first data stream of our device
  r->FX_SetVStream(0, r->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  // Render the two triangles from the data stream
  hr = dv->DrawPrimitive(D3DPT_TRIANGLEFAN, nOffs, 2);

  r->m_nPolygons+=12;
}
*/
CREOcclusionQuery::~CREOcclusionQuery()
{
  mfReset();
}

void CREOcclusionQuery::mfReset()
{


#if defined (DIRECT3D9)
  LPDIRECT3DQUERY9  pVizQuery = (LPDIRECT3DQUERY9)m_nOcclusionID;
  SAFE_RELEASE(pVizQuery)
#elif defined (DIRECT3D10)
  ID3D11Query  *pVizQuery = (ID3D11Query*)m_nOcclusionID;
  SAFE_RELEASE(pVizQuery)
#endif
  
  m_nOcclusionID = 0;
  m_nDrawFrame = 0;
  m_nCheckFrame = 0;
  m_nVisSamples = 0;
}

uint32 CREOcclusionQuery::m_nQueriesPerFrameCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultNowCounter = 0;
uint32 CREOcclusionQuery::m_nReadResultTryCounter = 0;

bool CREOcclusionQuery::mfDraw(CShader *ef, SShaderPass *sfm)
{ 
	PROFILE_FRAME(CREOcclusionQuery::mfDraw);
 
  CD3D9Renderer *r = gcpRendD3D;

  CShader *pSh = CShaderMan::m_ShaderOcclTest;
  if( !pSh || pSh->m_HWTechniques.empty())
    return false;

  if (!(r->m_Features & RFT_OCCLUSIONTEST))
  { // If not supported
    m_nVisSamples = r->GetWidth()*r->GetHeight();
    return true;
  }

  int w =  r->GetWidth();
  int h =  r->GetHeight();

  if (!m_nOcclusionID)
  {
    HRESULT hr;


#if defined (DIRECT3D9) || defined(OPENGL)
    // Create visibility query
    LPDIRECT3DQUERY9  pVizQuery = NULL;
    
    hr = r->m_pd3dDevice->CreateQuery (D3DQUERYTYPE_OCCLUSION, &pVizQuery);
    if (pVizQuery)
      m_nOcclusionID = (UINT_PTR)pVizQuery;
#elif defined (DIRECT3D10)

    ID3D11Query  *pVizQuery = NULL;
    D3D11_QUERY_DESC qdesc;
    qdesc.MiscFlags = 0; //D3D11_QUERY_MISC_PREDICATEHINT;
    qdesc.Query = D3D11_QUERY_OCCLUSION;
    hr = r->m_pd3dDevice->CreateQuery( &qdesc, &pVizQuery );
    if (pVizQuery)
      m_nOcclusionID = (UINT_PTR)pVizQuery;
#endif
  }

	int nFrame = r->GetFrameID();

  if (m_nDrawFrame != nFrame)
  { // draw test box
    if (m_nOcclusionID)
    {


#if defined (DIRECT3D9) || defined(OPENGL)
      D3DQuery*  pVizQuery = (D3DQuery*)m_nOcclusionID;
      pVizQuery->Issue (D3DISSUE_BEGIN);
#elif defined (DIRECT3D10)
			D3DQuery*  pVizQuery = (D3DQuery*)m_nOcclusionID;
			r->m_pd3dDeviceContext->Begin(pVizQuery);
#endif

      /////////////////////////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////

      int nVertOffs, nIndOffs;

      static t_arrDeferredMeshIndBuff arrDeferredInds;
      static t_arrDeferredMeshVertBuff arrDeferredVerts;
			arrDeferredVerts.resize(0);
			arrDeferredInds.resize(0);
      r->CreateDeferredUnitBox(arrDeferredInds, arrDeferredVerts);

      //allocate vertices
      SVF_P3F_C4B_T2F  *pVerts( (SVF_P3F_C4B_T2F *) r->GetVBPtr( arrDeferredVerts.size(), nVertOffs, POOL_P3F_COL4UB_TEX2F ) );
      memcpy( pVerts, &arrDeferredVerts[0], arrDeferredVerts.size()*sizeof(SVF_P3F_C4B_T2F ) );
      r->UnlockVB( POOL_P3F_COL4UB_TEX2F ); 

      //allocate indices
      uint16 *pInds = r->GetIBPtr(arrDeferredInds.size(), nIndOffs);
      memcpy( pInds, &arrDeferredInds[0], sizeof(uint16)*arrDeferredInds.size() );
      r->UnlockIB();

      r->FX_SetVStream( 0,r->m_pVB[ POOL_P3F_COL4UB_TEX2F ], 0, sizeof( SVF_P3F_C4B_T2F ) );
      r->FX_SetIStream(r->m_pIB);

      /////////////////////////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////

      r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_matView->Push();
      Matrix34 mat;
      mat.SetIdentity();
      mat.SetScale(m_vBoxMax-m_vBoxMin,m_vBoxMin);

			const Matrix44 cTransPosed = GetTransposed44(Matrix44(mat));
      r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_matView->MultMatrixLocal(&cTransPosed);
      r->EF_DirtyMatrix();

      uint32 nPasses;
      pSh->FXSetTechnique("General");
      pSh->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      pSh->FXBeginPass(0);





      r->FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F );

      int nPersFlagsSave = r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags;
      int nObjFlagsSave = r->m_RP.m_ObjFlags;
      CRenderObject *pCurObjectSave = r->m_RP.m_pCurObject;
      CShader *pShaderSave = r->m_RP.m_pShader;
			SShaderTechnique *pCurTechniqueSave = r->m_RP.m_pCurTechnique;


      r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_FP_DIRTY | RBPF_FP_MATRIXDIRTY;
      r->m_RP.m_ObjFlags &= ~FOB_TRANS_MASK;
      r->m_RP.m_pCurObject = r->m_RP.m_TempObjects[0][0];
      r->m_RP.m_pCurInstanceInfo = &r->m_RP.m_pCurObject->m_II;
      r->m_RP.m_pShader = pSh;
      r->m_RP.m_pCurTechnique = pSh->m_HWTechniques[0];
      r->EF_SetState(GS_COLMASK_NONE|GS_DEPTHFUNC_LEQUAL); 
      r->SetCullMode(R_CULL_NONE);

      r->FX_Commit();

#if defined (DIRECT3D9) || defined(OPENGL)
      r->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, nVertOffs, 0, arrDeferredVerts.size(), nIndOffs, arrDeferredInds.size()/3);
#elif defined (DIRECT3D10)
      r->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      r->m_pd3dDeviceContext->DrawIndexed(arrDeferredInds.size(), nIndOffs, nVertOffs);
#endif

      r->m_RP.m_PS[r->m_RP.m_nProcessThreadID].m_nPolygons[r->m_RP.m_nPassGroupDIP] += arrDeferredInds.size()/3;
      r->m_RP.m_PS[r->m_RP.m_nProcessThreadID].m_nDIPs[r->m_RP.m_nPassGroupDIP]++;

      pSh->FXEndPass();
      pSh->FXEnd();

      r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_matView->Pop();
      r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags = nPersFlagsSave;
      r->m_RP.m_ObjFlags = nObjFlagsSave;
      r->m_RP.m_pCurObject = pCurObjectSave;
      r->m_RP.m_pCurInstanceInfo = &r->m_RP.m_pCurObject->m_II;
      r->m_RP.m_pShader = pShaderSave;
      r->m_RP.m_pCurTechnique = pCurTechniqueSave;


#if defined (DIRECT3D9) || defined(OPENGL)
      pVizQuery->Issue (D3DISSUE_END);
#elif defined (DIRECT3D10)
			r->m_pd3dDeviceContext->End(pVizQuery);
#endif

			CREOcclusionQuery::m_nQueriesPerFrameCounter++;
    }

    m_nDrawFrame = nFrame;
  }

  //gcpRendD3D->ResetToDefault();
  //gcpRendD3D->FX_ResetPipe(); 
  return true;
}

bool CREOcclusionQuery::mfReadResult_Now()
{
	int nFrame = gcpRendD3D->GetFrameID();




#if defined (DIRECT3D9)
	LPDIRECT3DQUERY9  pVizQuery = (LPDIRECT3DQUERY9)m_nOcclusionID;
	if (pVizQuery)
	{
		HRESULT hRes = S_FALSE;
		while(hRes==S_FALSE)
			hRes = pVizQuery->GetData((void *) &m_nVisSamples, sizeof(DWORD), D3DGETDATA_FLUSH);

		if(hRes == S_OK)
			m_nCheckFrame = nFrame;
	}
#elif defined (DIRECT3D10)
  ID3D11Query  *pVizQuery = (ID3D11Query*)m_nOcclusionID;
  if (pVizQuery)
  {
    HRESULT hRes = S_FALSE;
    while(hRes==S_FALSE)
			hRes = gcpRendD3D->m_pd3dDeviceContext->GetData(pVizQuery, (void *) &m_nVisSamples, sizeof(uint64), 0);

    if(hRes == S_OK)
      m_nCheckFrame = nFrame;
  }
#endif

	m_nReadResultNowCounter ++;

	return (m_nCheckFrame == nFrame);
}

bool CREOcclusionQuery::mfReadResult_Try()
{
  return gRenDev->m_pRT->RC_OC_ReadResult_Try(this);
}
bool CREOcclusionQuery::RT_ReadResult_Try()
{
	PROFILE_FRAME(CREOcclusionQuery::mfReadResult_Try);

	int nFrame = gcpRendD3D->GetFrameID();








#if defined (DIRECT3D9)
  LPDIRECT3DQUERY9  pVizQuery = (LPDIRECT3DQUERY9)m_nOcclusionID;
  HRESULT hRes = S_OK;
	if (pVizQuery)
	{
		hRes = pVizQuery->GetData((void *) &m_nVisSamples, sizeof(DWORD), 0);

		if (hRes == S_OK)
			m_nCheckFrame = nFrame;
	}
#elif defined (DIRECT3D10)
  ID3D11Query  *pVizQuery = (ID3D11Query*)m_nOcclusionID;
  if (pVizQuery)
  {
    HRESULT hRes = S_FALSE;
    hRes = gcpRendD3D->m_pd3dDeviceContext->GetData(pVizQuery, (void *) &m_nVisSamples, sizeof(uint64), D3D11_ASYNC_GETDATA_DONOTFLUSH);

    if (hRes == S_OK)
      m_nCheckFrame = nFrame;
  }
#endif

	m_nReadResultTryCounter++;

#ifdef DO_RENDERLOG
  if (!m_nVisSamples)
  {
    if (CRenderer::CV_r_log)
      gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "OcclusionQuery: Water is not visible\n");
  }
  else
  {
    if (CRenderer::CV_r_log)
      gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "OcclusionQuery: Water is visible (%d samples)\n", m_nVisSamples);
  }
#endif

	return (m_nCheckFrame == nFrame);
}

void CRETempMesh::mfReset()
{
  gRenDev->m_DevBufMan.ReleaseVBuffer(m_pVBuffer);
  gRenDev->m_DevBufMan.ReleaseIBuffer(m_pIBuffer);
  m_pVBuffer = NULL;
  m_pIBuffer = NULL;
}

bool CRETempMesh::mfPreDraw(SShaderPass *sl)
{
  CVertexBuffer *vb = m_pVBuffer;
  CIndexBuffer *ib = m_pIBuffer;
  CD3D9Renderer *rd = gcpRendD3D;

  if (!m_pVBuffer || !m_pIBuffer)
    return false;

  if (!m_nIndices)
    return false;

  int32 nVBOffs;
  D3DVertexBuffer *pVB = rd->m_DevBufMan.GetD3DVB(vb->m_VS.m_nDevID, &nVBOffs);
  int32 nIBOffs;
  D3DIndexBuffer *pIB = rd->m_DevBufMan.GetD3DIB(ib->m_VS.m_nDevID, &nIBOffs);

  HRESULT h = rd->FX_SetVStream(0, pVB, nVBOffs, CRenderMesh2::m_cSizeVF[vb->m_eVF], rd->m_RP.m_ReqStreamFrequence[0]);
  h = rd->FX_SetIStream(pIB);

  return true;
}


bool CRETempMesh::mfDraw(CShader *ef, SShaderPass *sl)
{
  CD3D9Renderer *r = gcpRendD3D;
  if (!m_pVBuffer || !m_pIBuffer)
    return false;

  // Hardware shader
  int nPrimType = R_PRIMV_TRIANGLES;
  r->FX_DrawIndexedMesh(nPrimType);

  return true;
}

bool CREMesh::mfPreDraw(SShaderPass *sl)
{
	assert(m_pRenderMesh);
	if (!m_pRenderMesh)
		return false;
	PROFILE_LABEL_SHADER(m_pRenderMesh->GetSourceName() ? m_pRenderMesh->GetSourceName() : "Unknown mesh-resource name");
  CRenderMesh2 *pRM = m_pRenderMesh->_GetVertexContainer();
  // Should never happen. Video buffer is missing
  if (!pRM->_HasVBStream(VSF_GENERAL))
    return false;
  CD3D9Renderer *rd = gcpRendD3D;
  int i;
#ifdef _DEBUG
  for (i=0; i<VSF_NUM; i++)
  {
    if (!i || (rd->m_RP.m_FlagsStreams_Stream & (1<<i)))
    {
      if (pRM->_IsVBStreamLocked(i))
      {
        assert(0);
      }
    }
  }
#endif
  HRESULT h;

  int nOffs;
  D3DVertexBuffer *pVB = rd->m_DevBufMan.GetD3DVB(pRM->_GetVBStream(VSF_GENERAL), &nOffs);
  if (!pVB)
    pVB = rd->m_pVB[POOL_P3F_COL4UB_TEX2F];
  h = rd->FX_SetVStream(0, pVB, nOffs, pRM->GetStreamStride(VSF_GENERAL), rd->m_RP.m_ReqStreamFrequence[0]);

  int nThreadID = rd->m_RP.m_nProcessThreadID;

  for (i=1; i<VSF_NUM; i++)
  {
    if (rd->m_RP.m_FlagsStreams_Stream & (1<<i))
    {
      rd->m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_USESTREAM<<i;

      pVB = rd->m_DevBufMan.GetD3DVB(pRM->_GetVBStream(i), &nOffs);
      if (!pVB && i == VSF_TANGENTS)
        pVB = rd->m_pVB[2];
      h = rd->FX_SetVStream(i, pVB, nOffs, pRM->GetStreamStride(i), rd->m_RP.m_ReqStreamFrequence[i]);
    }
    else
    if (rd->m_RP.m_TI[nThreadID].m_PersFlags & (RBPF_USESTREAM<<i))
    {
      rd->m_RP.m_TI[nThreadID].m_PersFlags &= ~(RBPF_USESTREAM<<i);
      h = rd->FX_SetVStream(i, NULL, 0, 0, 1);
    }
  }

  D3DIndexBuffer *pIB = rd->m_DevBufMan.GetD3DIB(m_pRenderMesh->_GetIBStream(), &nOffs);
  assert(pIB);
  h = rd->FX_SetIStream(pIB);
  rd->m_RP.m_IndexOffset = nOffs>>1;

#if defined(CRY_DXPS_RASTERTHREAD)
	if(CD3D9Renderer::UsePS3SoftwareRastizer())
	{
		int32 IdxCount = m_pRenderMesh->_GetNumInds();
		uint16* pIdxBuffer = m_pRenderMesh->LockIB(FSL_READ);
		rd->m_pd3dDevice->SoftRast().IndexBuffer(pIdxBuffer,IdxCount);

		int32 VtxStride = 0;
		void* pVtxBuffer	=	m_pRenderMesh->GetPosPtr(VtxStride, FSL_READ);
		const int	VCount	=	m_pRenderMesh->_GetNumVerts();
		rd->m_pd3dDevice->SoftRast().VertexBuffer(pVtxBuffer,VtxStride,VCount);
	}
#endif

  return true;
}


bool CREMesh::mfDraw(CShader *ef, SShaderPass *sl)
{
  CD3D9Renderer *r = gcpRendD3D;
  
  CRenderMesh2 *pRM = m_pRenderMesh;
  if (ef->m_HWTechniques.Num())
  {
#if defined(CRY_DXPS_RASTERTHREAD)
		if(CD3D9Renderer::UsePS3SoftwareRastizer() && r->m_RP.m_RendNumGroup<0 && (r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
		{
			r->m_pd3dDevice->SoftRast().Draw();
			return true;
		}
#endif
    r->FX_DrawIndexedMesh(r->m_RP.m_RendNumGroup>=0 ? R_PRIMV_HWSKIN_GROUPS : pRM->_GetPrimitiveType());    
  }
  
  return true;
}

//===================================================================================

bool CREFlare::mfCheckVis(CRenderObject *obj)
{






  CD3D9Renderer *rd = gcpRendD3D;
  assert(rd->m_pRT->IsRenderThread());
  
  if( !obj->m_pLight )
    return false;

  Vec3 vOrg = obj->GetTranslation();
  bool bVis = false;
  bool bSun = false;

  Vec3 v;  
  v = vOrg - rd->GetRCamera().Orig;  
  if( rd->GetRCamera().ViewDir().Dot(v) < 0.0f)
    return false;

  int32 vp[4];
  rd->GetViewport(&vp[0], &vp[1], &vp[2], &vp[3]);

  Vec3 vx0, vy0;  
  float dist = v.GetLength();

  SRenderObjData *pOD = obj->GetObjData(rd->m_RP.m_nProcessThreadID);
  if (!pOD)
    return false;

  CREOcclusionQuery *pRE = (CREOcclusionQuery *)pOD->m_pRE;
  if (!pRE || m_nFrameQuery != rd->m_nFrameReset || !pRE->m_nOcclusionID)
  {
    m_nFrameQuery = rd->m_nFrameReset;
    // Create visibility queries
#if defined (DIRECT3D9) || defined (OPENGL)
    LPDIRECT3DQUERY9  pVizQuery = NULL;
    HRESULT hr = gcpRendD3D->m_pd3dDevice->CreateQuery (D3DQUERYTYPE_OCCLUSION, &pVizQuery);
#elif defined (DIRECT3D10)
		ID3D11Query* pVizQuery(0);
		D3D11_QUERY_DESC desc;
		desc.Query = D3D11_QUERY_OCCLUSION;
		desc.MiscFlags = 0/*D3D11_QUERY_MISC_PREDICATEHINT*/;
		HRESULT hr(gcpRendD3D->m_pd3dDevice->CreateQuery(&desc, &pVizQuery));
		assert(SUCCEEDED(hr));
#endif
    if (pVizQuery)
    {
      if (!pRE)
        pOD->m_pRE = rd->EF_CreateRE(eDATA_OcclusionQuery);
      pRE = (CREOcclusionQuery *)pOD->m_pRE;
      if (pRE)
      {
        assert(!pRE->m_nOcclusionID);
        pRE->m_nOcclusionID = (UINT_PTR)pVizQuery;
        pRE->m_nVisSamples = 0;
      }
    }
  }
  else if (pRE)
  {
    int nFrame = rd->GetFrameID(); 

    if (pRE->m_nCheckFrame != nFrame)
    {
      if(pRE->m_nCheckFrame)
      {
        pRE->m_nVisSamples=0;
#if defined (DIRECT3D9) || defined (OPENGL)
        LPDIRECT3DQUERY9 pVizQuery = (LPDIRECT3DQUERY9)pRE->m_nOcclusionID;
#elif defined (DIRECT3D10)
				ID3D11Query* pVizQuery = (ID3D11Query*)pRE->m_nOcclusionID;
#endif
				// Block until we have a query result
        if (pVizQuery)
        {
          float fTime = iTimer->GetAsyncCurTime();
          bool bInfinite = false;

#if defined (DIRECT3D9) || defined (OPENGL)
					while (pVizQuery->GetData(&pRE->m_nVisSamples, sizeof(int32), D3DGETDATA_FLUSH) == S_FALSE)
					{
#elif defined (DIRECT3D10)
					uint64 numVisSamples(0);
					while (gcpRendD3D->m_pd3dDeviceContext->GetData(pVizQuery, (void *)&numVisSamples, sizeof(uint64), 0) == S_FALSE)
					{
						pRE->m_nVisSamples = (int32) numVisSamples;
#endif
            float fDif = iTimer->GetAsyncCurTime() - fTime;
            if (fDif > 0.5f)
            {
              // 5 seconds in the loop
              bInfinite = true;
              break;
            }
          }
#if defined (DIRECT3D10)
          // make sure to update..
          pRE->m_nVisSamples = (int32) numVisSamples;
#endif

          rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_fOcclusionTime += iTimer->GetAsyncCurTime()-fTime;
          if (bInfinite)
            iLog->Log("Error: Seems like infinite loop in flare occlusion query");
        }
      }
      pRE->m_nCheckFrame = nFrame;
    }
  }

	if (pRE)
	{
		float fMinWidth = 8.0f / (float)rd->GetWidth();
		float fMinHeight = 8.0f / (float)rd->GetHeight();     

		// Clamp minimum size for occlusion query else we'll get accuracy issues like flickering
		vx0 = rd->GetRCamera().X*(dist*0.01f);
		vy0 = rd->GetRCamera().Y*(dist*0.01f);

		CTexture::s_ptexWhite->Apply(0);

		UCol color;
		color.dcolor = ~0;
		int nOffs;

		SVF_P3F_C4B_T2F *vQuad = (SVF_P3F_C4B_T2F *)rd->GetVBPtr(4, nOffs);

		vQuad[0].xyz = vOrg + vx0 + vy0;
		vQuad[0].st = Vec2(0.0f, 0.0f);
		vQuad[0].color = color;

		vQuad[1].xyz = vOrg + vx0 - vy0;
		vQuad[1].st = Vec2(0.0f, 1.0f);
		vQuad[1].color = color;

		vQuad[2].xyz = vOrg - vx0 + vy0; 
		vQuad[2].st = Vec2(1.0f, 0.0f);
		vQuad[2].color = color;

		vQuad[3].xyz = vOrg - vx0 - vy0;
		vQuad[3].st = Vec2(1.0f, 1.0f);
		vQuad[3].color = color;

		rd->UnlockVB();

		// Render the 2 triangles from the data stream
#if defined (DIRECT3D9) || defined (OPENGL)  
		LPDIRECT3DQUERY9 pVizQuery = (LPDIRECT3DQUERY9) pRE->m_nOcclusionID;
#elif defined (DIRECT3D10)
		ID3D11Query* pVizQuery = (ID3D11Query*)pRE->m_nOcclusionID;
#endif

		rd->FX_SetVStream(0, rd->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
    rd->FX_SetFPMode();

		rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);

		rd->EF_SetState(GS_COLMASK_NONE);  
		rd->SetCullMode(R_CULL_NONE);

		CTexture::s_ptexWhite->Apply(0); 

#if defined (DIRECT3D9) || defined (OPENGL)  
		HRESULT hr(S_OK);
		hr = pVizQuery->Issue(D3DISSUE_BEGIN);
		rd->m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2); 
		hr = pVizQuery->Issue(D3DISSUE_END);
#elif defined (DIRECT3D10)
		rd->m_pd3dDeviceContext->Begin(pVizQuery);
		rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		rd->m_pd3dDeviceContext->Draw(4, nOffs);
		rd->m_pd3dDeviceContext->End(pVizQuery);
#endif

		Vec3 ProjV[4];
		for (int n=0; n<4; n++)
		{
      Vec3 V = vQuad[n].xyz;
			mathVec3Project(&ProjV[n], &V, vp, &rd->m_ProjMatrix, &rd->m_CameraMatrix, &rd->m_IdentityMatrix);
		}

		float nX = fabsf(ProjV[0].x - ProjV[2].x);
		float nY = fabsf(ProjV[0].y - ProjV[2].y);  
		float area = max(0.1f, nX * nY);          

		float fIntens = (float)pRE->m_nVisSamples/ (area);   

		// workaround: Make sure no flickering occurs due to inaccuracy
		fIntens = clamp_tpl<float>(fIntens*2.0f, 0.0f, 1.0f);  
		if( fIntens < 0.9f) 
			fIntens = 0.0f;

		float fFading = CRenderer::CV_r_coronafade;
		// Accumulate previous frame results and blend in results
		pOD->m_fTempVars[0] = pOD->m_fTempVars[1];
		pOD->m_fTempVars[1] = pOD->m_fTempVars[0] * (1.0f-fFading)  + fIntens * fFading;  
	}

  return true;
}

// TODO: - refactor entire thing..
void CREFlare::mfDrawCorona(CShader *ef, ColorF &col)
{
  CD3D9Renderer *rd = gcpRendD3D;
  CRenderObject *obj = rd->m_RP.m_pCurObject;

  // This shouldn't happen, but just in case...
  CShader *pShader= CShaderMan::m_ShaderLightFlares;
  if(!CShaderMan::m_ShaderLightFlares)
    return;

  if(m_Importance > CRenderer::CV_r_coronas)
    return;

  STexState pTexState;
  pTexState.SetFilterMode(FILTER_LINEAR);        
  pTexState.SetClampMode(1, 1, 1);  


  Vec3 vOrg, v;
  vOrg = obj->GetTranslation(); 
  v = vOrg - rd->GetRCamera().Orig;  
  if( rd->GetRCamera().ViewDir().Dot(v) < 0.0f)
    return;

  int vp[4];
  rd->GetViewport(&vp[0], &vp[1], &vp[2], &vp[3]);

  Vec3 vScr;
  mathVec3Project(&vScr, &vOrg, vp, &rd->m_ProjMatrix, &rd->m_CameraMatrix, &rd->m_IdentityMatrix);

  float fScaleCorona = 0.03f;//m_fScaleCorona;  
  if(!obj->m_pLight)
    return;

  if(obj->m_pLight && obj->m_pLight->m_fCoronaScale)
   fScaleCorona *= obj->m_pLight->m_fCoronaScale;

  Vec3 vx0, vy0, vPrev;
  vPrev = vOrg - rd->m_prevCamera.GetPosition();    
  
  // Get normalized distance  
  float fD = vPrev.GetLength();
  float fP = rd->m_prevCamera.GetFarPlane();

  float dist = fD / fP;

  // Idea: Maybe also adjusting corona size according to it's brightness would look nice  
  float fDistSizeFactor = m_fDistSizeFactor * obj->m_pLight->m_fCoronaDistSizeFactor;
	if(fDistSizeFactor <= 0.0f)
		fScaleCorona = 0.0f;
  else if(fDistSizeFactor != 1.0f)
    fScaleCorona *= cry_powf(1.0f - dist, 1.0f / fDistSizeFactor); 
  
  float fDecay = col[3];
  fDecay *= CRenderer::CV_r_coronacolorscale;  

  float fDistIntensityFactor = m_fDistIntensityFactor * obj->m_pLight->m_fCoronaDistIntensityFactor;
	if(fDistIntensityFactor <= 0.0f)
		fDecay = 0.0f;
  else if(fDistIntensityFactor != 1.0f)
    fDecay *= cry_powf(1.0f - dist, 1.0f / fDistIntensityFactor);

  if(fDecay <= 0.001f)
    return;
  else
  if(fDecay > 1.0f)
    fDecay = 1.0f;

    // Get flare color from material and light color
    ColorF pColor(1,1,1,1);  
    if(obj->m_pLight)
      pColor = obj->m_pLight->m_Color;

    if (rd->m_RP.m_pShaderResources && rd->m_RP.m_pShaderResources->m_Constants[eHWSC_Pixel].size())
    {    
      ColorF *pSrc = (ColorF *)&rd->m_RP.m_pShaderResources->m_Constants[eHWSC_Pixel][0];
      pColor *= pSrc[PS_DIFFUSE_COL];
    }

    ColorF c(1,1,1,1);
    float fMax = max(col.r, max(col.b, col.g));
    if(fMax > 1.0f)
      col.NormalizeCol(c);
    else
      c = col;  

    c*= pColor;

    // apply attenuation
    c.r *= fDecay;
    c.g *= fDecay;
    c.b *= fDecay;
    c.a = fDecay;

    // Set orthogonal ViewProj matrix
    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Push();
    mathMatrixOrtho(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->GetTop(), (float)rd->GetWidth(), (float)rd->GetHeight(), -20.0, 0.0);

    rd->PushMatrix();
    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

    // Render a normal flare (this should be optimized into just a single render pass, not one per flare)
    uint32 nPasses=0;     
		static CCryName techName("LightFlare");
    pShader->FXSetTechnique(techName);
    pShader->FXBegin(&nPasses, FEF_DONTSETSTATES|FEF_DONTSETTEXTURES);
    pShader->FXBeginPass(0);

    Vec4 pParams= Vec4(c.r, c.g, c.b, c.a);            
		static CCryName cFlareColorName("cFlareColor");
    pShader->FXSetPSFloat(cFlareColorName, &pParams, 1);

    // Need to set states from code, somehow from shader they are not set properly        
    rd->EF_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE |GS_BLDST_ONE);       
    rd->SetCullMode(R_CULL_NONE);   

    if(rd->m_RP.m_pShaderResources && rd->m_RP.m_pShaderResources->m_Textures[EFTT_DIFFUSE] && rd->m_RP.m_pShaderResources->m_Textures[EFTT_DIFFUSE]->m_Sampler.m_pTex)
      rd->m_RP.m_pShaderResources->m_Textures[EFTT_DIFFUSE]->m_Sampler.m_pTex->Apply(0, CTexture::GetTexState(pTexState));

    // Size is texture size x scale (corona scale/base scale(i used 0.2 as reference value))
    float fSize= (128.0f)*(fScaleCorona/0.2f)* CRenderer::CV_r_coronasizescale;  // texture size 
    rd->DrawQuad(vScr.x-fSize, vScr.y-fSize, vScr.x+fSize, vScr.y+fSize, c);        

    pShader->FXEndPass(); 
    pShader->FXEnd();

    // Restore data
    rd->PopMatrix();
    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Pop();
    rd->EF_SelectTMU(0);

    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += 2;
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
}

static float sInterpolate(float& pprev, float& prev, float& next, float& nnext, float ppweight, float pweight, float nweight, float nnweight)
{
  return pprev*ppweight + prev*pweight + next*nweight + nnext*nnweight;
}

static float sSpline(float x)
{
  float fX = fabsf(x);

  if(fX > 2.0f)
    return 0;
  if(fX > 1.0f)
    return (2.0f-fX)*(2.0f-fX)*(2.0f-fX)/6.0f;
  return 2.0f/3.0f-fX*fX+0.5f*fX*fX*fX;
}

bool CREFlare::mfDraw(CShader *ef, SShaderPass *sfm)
{
  CD3D9Renderer *rd = gcpRendD3D;
  assert(rd->m_pRT->IsRenderThread());

  if (!CRenderer::CV_r_coronas)
    return false;

#if defined( PS3 )
	// todo: occlusion queries for flares needs to be implemented
	return false;
#endif

  int nThreadID = rd->m_RP.m_nProcessThreadID;
  rd->m_RP.m_TI[nThreadID].m_matView->LoadMatrix((Matrix44*)rd->m_CameraMatrix.GetData());

  CRenderObject *obj = rd->m_RP.m_pCurObject;
  SRenderObjData *pOD = obj->GetObjData(nThreadID);
    
  if (!mfCheckVis(obj))
    return false;

  // Restore current object
  rd->m_RP.m_pCurObject = obj;    
  rd->m_RP.m_pCurInstanceInfo = &rd->m_RP.m_pCurObject->m_II;
  
  float fBrightness = pOD->m_fTempVars[1];
  fBrightness = clamp_tpl<float>(fBrightness, 0.0f, 1.0f);    
  if(!fBrightness)
  {
    return false;
  }

  obj->m_II.m_AmbColor.r = obj->m_II.m_AmbColor.g = obj->m_II.m_AmbColor.b = 1;  
  obj->m_II.m_AmbColor.a = fBrightness;

  mfDrawCorona(ef, obj->m_II.m_AmbColor);
  
  // Disabled for now as it is not working - lens-flare
  // mfDrawFlares(ef, obj->m_Color);

  return true;
} 

void CRenderMesh2::DrawImmediately()
{
  CD3D9Renderer *rd = gcpRendD3D;

  HRESULT hr = rd->FX_SetVertexDeclaration(0, _GetVertexFormat());

  if (FAILED(hr))
  {
    assert(!"CRenderMesh::DrawImmediately failed");
    return;
  }

  // set vertex and index buffer
  CheckUpdate(_GetVertexFormat(), 0);

  int vbOffset(0);
  int ibOffset(0);
  D3DVertexBuffer *pVB = rd->m_DevBufMan.GetD3DVB(_GetVBStream(VSF_GENERAL), &vbOffset);
  D3DIndexBuffer *pIB = rd->m_DevBufMan.GetD3DIB(_GetIBStream(), &ibOffset);
  assert(pVB);
  assert(pIB);

  if (!pVB || !pIB)
  {
    assert(!"CRenderMesh::DrawImmediately failed");
    return;
}

  hr =  rd->FX_SetVStream(0, pVB, vbOffset, GetStreamStride(VSF_GENERAL));

  ibOffset /= sizeof(uint16);
  hr = rd->FX_SetIStream(pIB);

  // draw sky dome
#if defined (DIRECT3D9) || defined(OPENGL)
  hr = rd->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _GetNumVerts(), ibOffset, _GetNumInds() / 3);
#elif defined (DIRECT3D10)
  rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  rd->m_pd3dDeviceContext->DrawIndexed(_GetNumInds(), ibOffset, 0);
#endif
  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += _GetNumInds() / 3;
  rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
}

bool CREHDRProcess::mfDraw(CShader *ef, SShaderPass *sfm)
{
  CD3D9Renderer *rd = gcpRendD3D;
  assert(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR || rd->m_RP.m_CurState & GS_WIREFRAME);
  if (!(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR))
    return false;

  rd->FX_HDRPostProcessing();
  return true;
}

bool CREBeam::mfDraw(CShader *ef, SShaderPass *sl)
{  
  CD3D9Renderer *rd = gcpRendD3D;

  if (SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID]!=1)
    return false;

  using namespace NPostEffects;

  EShaderQuality nShaderQuality = (EShaderQuality) gcpRendD3D->EF_GetShaderQuality(eST_FX);
  ERenderQuality nRenderQuality = gRenDev->m_RP.m_eQuality;
  bool bLowSpecShafts = (nShaderQuality == eSQ_Low) || (nRenderQuality == eRQ_Low);

  bool bUseOptBeams = CRenderer::CV_r_beams == 3 && (CRenderer::CV_r_HDRRendering || gRenDev->CV_r_PostProcess);
  
  CTexture *pShaftsRT = CTexture::s_ptexGlow;
  
  // If HDR active, render to floating point buffer instead
  if( CRenderer::CV_r_HDRRendering > 0 )
    pShaftsRT = CTexture::s_ptexHDRTargetScaled[2];
    
  if((!pShaftsRT && bUseOptBeams) || !rd->m_RP.m_pCurObject)   
    return 0;
  
  int vX, vY, vWidth, vHeight;
  rd->GetViewport(&vX, &vY, &vWidth, &vHeight);

  CRenderCamera cam;
  CRenderObject *pObj = rd->m_RP.m_pCurObject;
  CDLight *pLight = pObj->m_pLight;
  cam.Perspective(DEG2RAD(pLight->m_fLightFrustumAngle*2.0f), 1, 0.01f, pLight->m_fRadius); 
  Vec3 vForward = pObj->m_II.m_Matrix.GetColumn(0);
  Vec3 vUp = pObj->m_II.m_Matrix.GetColumn(2);
  cam.LookAt(pLight->m_Origin, pLight->m_Origin+vForward, vUp);
  Vec3 vPoints[8];
  cam.CalcVerts(vPoints);
  const CRenderCamera& rcam = rd->GetRCamera();
  int i;
  float fNearDist = FLT_MAX;
  float fFarDist = -FLT_MAX;
  Plane pNear;
  Plane pFar;

  for (i=0; i<8; i++)
  {
	  Plane p = Plane::CreatePlane(rcam.Z, vPoints[i]);
    float fDist = p.DistFromPlane(rcam.Orig);
    if (fNearDist > fDist)
    {
      fNearDist = fDist;
      pNear = p;
    }
    if (fFarDist < fDist)
    {
      fFarDist = fDist;
      pFar = p; 
    }
  }

  if (fFarDist <= 0)  
    return true;

  if( bLowSpecShafts || CRenderer::CV_r_beams == 4)
  {
#ifdef DO_RENDERLOG
    if (rd->m_LogFile)
      rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], " +++ Draw low spec beam for light '%s' (%.3f, %.3f, %.3f) +++ \n", pLight->m_sName ? pLight->m_sName : "<Unknown>", pLight->m_Origin[0], pLight->m_Origin[1], pLight->m_Origin[2]);
#endif

    CShader *pShader= CShaderMan::m_ShaderLightFlares;
    if(!CShaderMan::m_ShaderLightFlares)
      return false;

    rd->FX_ResetPipe();
    STexState pTexState;
    pTexState.SetFilterMode(FILTER_TRILINEAR);        
    pTexState.SetClampMode(1, 1, 1);  

    STexState pTexStateSec;
    pTexStateSec.SetFilterMode(FILTER_POINT);        
    pTexStateSec.SetClampMode(0, 0, 0);  
      
    int vp[4];
    rd->GetViewport(&vp[0], &vp[1], &vp[2], &vp[3]);

    // Get flare color from material and light color
    ColorF pColor = pLight->m_Color;

    if (rd->m_RP.m_pShaderResources && rd->m_RP.m_pShaderResources->m_Constants[eHWSC_Pixel].size())
    {    
      ColorF *pSrc = (ColorF *)&rd->m_RP.m_pShaderResources->m_Constants[eHWSC_Pixel][0];
      pColor *= pSrc[PS_DIFFUSE_COL];
    }

    Matrix44A mProjMatrix = rd->m_ProjMatrix;
    Matrix44A mCameraMatrix = rd->m_CameraMatrix;

    // Get light frustum bounding rectangle
    Vec2 pMin(vWidth, vHeight), pMax(0, 0);
    for( int p = 0; p < 8; ++p )
    {
      Vec3 vProj = Vec3(0,0,0);
      mathVec3Project(&vProj, &vPoints[p], vp, &mProjMatrix, &mCameraMatrix, &rd->m_IdentityMatrix);  	  
      pMin.x = min(pMin.x, vProj.x);
      pMin.y = min(pMin.y, vProj.y);
      pMax.x = max(pMax.x, vProj.x);
      pMax.y = max(pMax.y, vProj.y);
    }

    Vec2 pMed = (pMax + pMin) *0.5f;
    /*
    pMin.x = max(pMin.x, 0.0f);
    pMin.y = max(pMin.y, 0.0f);
    pMax.x = min(pMax.x, (float)vWidth);
    pMax.y = min(pMax.y, (float)vHeight);*/

    // Set orthogonal ViewProj matrix
    rd->Set2DMode(true, rd->GetWidth(), rd->GetHeight());

    // Render light beam flares
    uint32 nPasses=0;     
    static CCryName techName("LowSpecBeams"); 
    pShader->FXSetTechnique(techName);
    pShader->FXBegin(&nPasses, FEF_DONTSETSTATES|FEF_DONTSETTEXTURES);
    pShader->FXBeginPass(0);

    Vec4 pParams= Vec4(pColor.r, pColor.g, pColor.b, pColor.a);            
    static CCryName cFlareColorName("cFlareColor");
    
    rd->EF_SetState(GS_DEPTHFUNC_LEQUAL | GS_BLSRC_ONE |GS_BLDST_ONEMINUSSRCCOL);       
    rd->SetCullMode(R_CULL_NONE);   

    // Adjudst radius scale according to volume size on screen
    float fRadScale = (pMed - pMax).GetLength();
    fRadScale = min(fRadScale, (float) vWidth);

    // Adjust amount of planes according to light size
    int nNumPlanes = max( 10, int_round( floorf(pLight->m_fRadius*0.5f) ) );  
    float fPlaneStep = pLight->m_fRadius ;

    float fSizeStep = 1.0f / (float)nNumPlanes;
    for(int p = 0; p < nNumPlanes; ++p)
    {
      Vec3 vOrg= Vec3(0,0,0), v= Vec3(0,0,0);
      float fIncr =  ((float)p)/ (float)nNumPlanes;
      vOrg = pLight->m_Origin  + fPlaneStep *(fIncr*fIncr) * vForward;

      v = vOrg - rd->GetRCamera().Orig;  
      // cull
      if( rd->GetRCamera().ViewDir().Dot(v) < 0.0f)
        continue;

      float fInvRadius = pLight->m_fRadius;
      if (fInvRadius <= 0.0f)
        fInvRadius = 1.0f;

      fInvRadius = 1.0f / fInvRadius;

      // light position
      Vec3 pLightVec = pLight->m_Origin - vOrg;

      // compute attenuation
      pLightVec *= fInvRadius;
      float fAttenuation = clamp_tpl<float>(1.0f - (pLightVec.x * pLightVec.x + pLightVec.y * pLightVec.y + pLightVec.z * pLightVec.z), 0.0f, 1.0f);

      Vec3 vScr = Vec3(0,0,0);
      mathVec3Project(&vScr, &vOrg, vp, &mProjMatrix, &mCameraMatrix, &rd->m_IdentityMatrix);

      // Get normalized distance  
      float fD = v.GetLength();
      float fP = rd->GetRCamera().Far;
      float fDist =   min(100.0f*fD / fP, 1.0f);

      // compute size and attenuation factors
      float fShaftSliceSize = fRadScale* (min(pLight->m_fLightFrustumAngle * 2.0f, 60.0f)/90.0f) * 0.5f;
      float fDistSizeFactor = fShaftSliceSize/fDist; 
      float fNearPlaneSoftIsec = min(fD/2.0f, 1.0f); 
      
      pParams= Vec4(pColor.r, pColor.g, pColor.b, 1.0f)* fSizeStep * fNearPlaneSoftIsec * fAttenuation;
      pParams.w = fAttenuation;

      pShader->FXSetPSFloat(cFlareColorName, &pParams, 1);

      if( pLight->m_pLightImage )
        ((CTexture *)pLight->m_pLightImage)->Apply(0, CTexture::GetTexState(pTexState));
      else
        CTexture::s_ptexWhite->Apply(0, CTexture::GetTexState(pTexState));

      float fSize= fDistSizeFactor * (fSizeStep * ((float)p + 1.0f));

      //todo: merge geometry
      rd->DrawQuad(vScr.x-fSize, vScr.y-fSize, vScr.x+fSize, vScr.y+fSize, pColor, vScr.z);        
    }

    pShader->FXEndPass(); 
    pShader->FXEnd();

    rd->Set2DMode(false, rd->GetWidth(), rd->GetHeight());
    rd->EF_SelectTMU(0);

    return true;
  }

  if (gRenDev->m_LogFile)
    gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], " +++ Draw beam for light '%s' (%.3f, %.3f, %.3f) +++ \n", pLight->m_sName ? pLight->m_sName : "<Unknown>", pLight->m_Origin[0], pLight->m_Origin[1], pLight->m_Origin[2]);


  CHWShader_D3D *curVS = (CHWShader_D3D *)sl->m_VShader;
  CHWShader_D3D *curPS = (CHWShader_D3D *)sl->m_PShader;

  if (!curPS || !curVS)
    return false;

  uint32 nCasters = 0;
  ShadowMapFrustum* pFr = NULL;
  SShaderTechnique *pTech = rd->m_RP.m_pCurTechnique;
  uint64 nCurrFlagShader_RT = rd->m_RP.m_FlagsShader_RT;
  

  // We will draw the slices in world space
  // So set identity object
  rd->m_RP.m_pCurObject = rd->m_RP.m_Objects[0];
  rd->m_RP.m_pCurInstanceInfo = &rd->m_RP.m_pCurObject->m_II;
  rd->m_RP.m_LPasses[0].nLights = 1;
  rd->m_RP.m_LPasses[0].pLights[0] = pObj->m_pLight;
  rd->m_RP.m_FrameObject++;
  CHWShader_D3D::mfSetLightParams(0);

  // Setup clip planes
  Plane clipPlanes[6];
  for (i=0; i<6; i++)
  {
    switch (i)
    {
    case 0:
      clipPlanes[i] = Plane::CreatePlane(vPoints[0], vPoints[7], vPoints[3]);
      break;
    case 1:
      clipPlanes[i] = Plane::CreatePlane(vPoints[1], vPoints[4], vPoints[0]);      
      break;
    case 2:
      clipPlanes[i] = Plane::CreatePlane(vPoints[2], vPoints[5], vPoints[1]);
      break;
    case 3:
      clipPlanes[i] = Plane::CreatePlane(vPoints[3], vPoints[6], vPoints[2]);
      break;
    case 4:
      clipPlanes[i] = Plane::CreatePlane(vPoints[0], vPoints[2], vPoints[1]);
      break;
    case 5:
      clipPlanes[i] = Plane::CreatePlane(vPoints[6], vPoints[4], vPoints[5]);
      break;
    }
    if (CRenderer::CV_r_beamssoftclip == 0)
    {
      Plane pTr = TransformPlane2(rd->m_InvCameraProjMatrix, clipPlanes[i]);
#if defined (DIRECT3D9) || defined (OPENGL)
      rd->m_pd3dDevice->SetClipPlane(i, &pTr.n[0]);
#elif defined (DIRECT3D10)
      assert(0);
#endif
    }
  }

  if (CRenderer::CV_r_beamssoftclip == 0)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    rd->m_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0x3f);
#elif defined (DIRECT3D10)
    assert(0);
#endif
  }

  float fDistNear = pNear.DistFromPlane(rcam.Orig);
  float fDistFar = pFar.DistFromPlane(rcam.Orig);
  float fDist = fDistFar - fDistNear;
  float fDistBetweenSlices = CRenderer::CV_r_beamsdistfactor;
  if (fDistNear > 100.0f) 
    fDistBetweenSlices *= fDistNear * 0.01f;
  if (fDistNear < 0) fDistNear = 0;
  if (fDistFar < 0)  fDistFar = 0;
  int nSlices = (int)(fabsf(fDistFar - fDistNear) / fDistBetweenSlices);
  if (nSlices > CRenderer::CV_r_beamsmaxslices)
    fDistBetweenSlices = fabsf(fDistFar - fDistNear) / (float)CRenderer::CV_r_beamsmaxslices;

  float fStartDist = pNear.d; 
  float fEndDist = pFar.d;
  if (pNear.d > pFar.d)
  {
    Exchange(fStartDist, fEndDist);
  }

  float fCurDist = fStartDist;
  float fIncrDist = fDistBetweenSlices;

  ColorF col;
  col.r = col.g = col.b = fDistBetweenSlices / fDist;    
  col.a = 1.0f;
  if (CRenderer::CV_r_beamshelpers)
    col = Col_White;

  SRenderShaderResources Res;
  SRenderShaderResources *pRes = rd->m_RP.m_pShaderResources;
  Res.m_Constants[eHWSC_Pixel].resize(4);
  Res.m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][0] = col[0];
  Res.m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][1] = col[1];
  Res.m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][2] = col[2];
  Res.m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][3] = col[3];

  rd->m_RP.m_pShaderResources = &Res;

  // Use optimized shafts
  if( bUseOptBeams )
  {    
    // Setup Shafts render-target
    int nWidth = pShaftsRT->GetWidth();
    int nHeight = pShaftsRT->GetHeight();

    rd->FX_PushRenderTarget(0, pShaftsRT, &gcpRendD3D->m_DepthBufferOrig);
    rd->RT_SetViewport(0, 0, nWidth, nHeight);      

    if( !( rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_LIGHTSHAFTS ) )
    {
      ColorF clearColor(0, 0, 0, 0);
      gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR, &clearColor);    
    }
  }

  rd->m_RP.m_FlagsShader_RT = 0;

  if (pLight && (pLight->m_Flags & DLF_CASTSHADOW_MAPS) && pLight->m_pShadowMapFrustums && pLight->m_pShadowMapFrustums[0]) 
  {   
    pFr = pLight->m_pShadowMapFrustums[0];
    if(pFr)
    {      
      rd->SetupShadowOnlyPass(0, pLight->m_pShadowMapFrustums[0], (rd->m_RP.m_ObjFlags & FOB_TRANS_MASK) ? &rd->m_RP.m_pCurObject->m_II.m_Matrix : NULL); 
      //reset bias param for first sampler
      rd->m_cEF.m_TempVecs[1][0] = 0.0f;  


      rd->m_RP.m_FlagsShader_RT &= ~( g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_CUBEMAP0] |
                                      g_HWSR_MaskBit[HWSR_POINT_LIGHT] | g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE]);

      rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

      if (!(pFr->m_Flags & DLF_DIRECTIONAL))
      {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_POINT_LIGHT ];  
      }

      if (pFr->bHWPCFCompare)
      {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
        //reset fOneDivFarDist param for first sampler
        //rd->m_cEF.m_TempVecs[2][0] = 1.f;
      }
      else 
      {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16]; 
      }

    }
  }
  rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_OBJ_IDENTITY];

  if (CRenderer::CV_r_beamshelpers)
  {
    rd->EF_SetState(GS_DEPTHWRITE | GS_WIREFRAME);
    CTexture::s_ptexWhite->Apply(0);
    CTexture::s_ptexWhite->Apply(1);

    SAuxGeomRenderFlags auxFlags;
    auxFlags.SetFillMode(e_FillModeWireframe);
    auxFlags.SetDepthTestFlag(e_DepthTestOff);
    rd->GetIRenderAuxGeom()->SetRenderFlags(auxFlags);

    ColorB cR = Col_Yellow;
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[0], cR, vPoints[4], cR, vPoints[7], cR);
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[0], cR, vPoints[7], cR, vPoints[3], cR);

    ColorB cT = Col_Yellow;
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[1], cT, vPoints[5], cT, vPoints[4], cT);
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[1], cT, vPoints[4], cT, vPoints[0], cT);

    ColorB cL = Col_Yellow;
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cL, vPoints[6], cL, vPoints[5], cL);
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cL, vPoints[5], cL, vPoints[1], cL);

    ColorB cB = Col_Yellow;
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cB, vPoints[3], cB, vPoints[7], cB);
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cB, vPoints[7], cB, vPoints[6], cB);

    ColorB cN = Col_Yellow;
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cN, vPoints[1], cL, vPoints[0], cN);
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cN, vPoints[0], cL, vPoints[3], cN);

    ColorB cF = Col_Yellow;
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cF, vPoints[1], cF, vPoints[0], cF);
    rd->GetIRenderAuxGeom()->DrawTriangle(vPoints[2], cF, vPoints[0], cF, vPoints[3], cF);
  }
  else
  {
    if( bUseOptBeams )
    {
      // make sure not to use depth test in optimized beams
      sl->m_RenderState |= GS_NODEPTHTEST;           
    }
    else
    {
      sl->m_RenderState &= ~GS_NODEPTHTEST;
    }

    
    // to save a little bit fillrate (not much..)
    sl->m_RenderState |= GS_ALPHATEST_GREATER;              
    sl->m_AlphaRef = 0;

    rd->FX_CommitStates(pTech, sl, false);
  }

  curPS->mfSet(HWSF_SETTEXTURES);
  curPS->mfSetParametersPI(NULL, NULL);

  curVS->mfSet(0);
  curVS->mfSetParametersPI(NULL, ef);

  CHWShader_D3D::mfBindGS(NULL, NULL);

  TArray<Vec3> vP;
  TArray<uint16> vI;
  float fDots[32];
  Plane p = pNear;
  
  int nCount( 0 );

  // update nearest current distance (to save some useless computations)
  //fCurDist = rcam.Near - (p.n.x * rcam.Orig.x +  p.n.y * rcam.Orig.y + p.n.z * rcam.Orig.z);

  while (fCurDist < fEndDist)
  {    
    p.d = fCurDist;
    float fFar = p.DistFromPlane(rcam.Orig);
    if (fFar > rcam.Near)
    {
      // Clip current slice by light frustum
      int nOffsV = vP.Num();

      // Define original full-screen polygon
      float FarZ = -fFar + 0.5f, FN = fFar/rcam.Near + 0.5f;     
      float fwL=rcam.wL*FN, fwR=rcam.wR*FN, fwB=rcam.wB*FN, fwT=rcam.wT*FN;
      
      Vec3* vNew = vP.AddIndex(4);
      vNew[0] = Vec3(fwR,fwT,FarZ);
      vNew[1] = Vec3(fwL,fwT,FarZ);
      vNew[2] = Vec3(fwL,fwB,FarZ);
      vNew[3] = Vec3(fwR,fwB,FarZ);

      for ( int j(0); j < 4; ++j )
      {
        vP[j+nOffsV] = rcam.CamToWorld(vP[j+nOffsV]); 
      }

      // Clip polygon by light frustum planes
      if (CRenderer::CV_r_beamssoftclip == 1)
      {
        for(int nPlaneIndex=0; nPlaneIndex<6; nPlaneIndex++)
        {
          uint32 nVertexIndex;
          for(nVertexIndex=0; nVertexIndex<vP.Num()-nOffsV; nVertexIndex++)
          {
            fDots[nVertexIndex] = clipPlanes[nPlaneIndex].DistFromPlane(vP[nVertexIndex+nOffsV]) - ( (nPlaneIndex==4)? pLight->m_fProjectorNearPlane:0);
          }
          uint32  nPrevVertexIndex = vP.Num() - nOffsV - 1;
          for(nVertexIndex=0; nVertexIndex<vP.Num()-nOffsV; nVertexIndex++)
          {
            float fDot = fDots[nVertexIndex];
            float fPrevDot = fDots[nPrevVertexIndex];

            // sign change, this plane clips this polys's edge
            if(fDot*fPrevDot < 0.0f)
            {
              Vec3 vPrevVertex = vP[nPrevVertexIndex+nOffsV];
              Vec3 vVertex = vP[nVertexIndex+nOffsV];

              float fFrac = - fPrevDot / (fDot - fPrevDot);     
               // add new vertex
              Vec3& v = vP.Insert(nVertexIndex+nOffsV);
              v = vPrevVertex + (vVertex - vPrevVertex) * fFrac;  

              // add new dot
              memmove(&fDots[nVertexIndex+1], &fDots[nVertexIndex], (vP.Num()-nOffsV-1-nVertexIndex)*sizeof(float));
              fDots[nVertexIndex] = 0.0f;
              nVertexIndex++;
            }

            nPrevVertexIndex = nVertexIndex;
          }

          // Remove clipped away vertices.
          int nDotVertexIndex = nOffsV;
          int nDots = vP.Num()-nOffsV;
          for(int nDotIndex=0; nDotIndex<nDots; nDotIndex++)
          {
            if(fDots[nDotIndex] < 0.0f)
            {
              vP.Remove(nDotVertexIndex);
            }
            else
              nDotVertexIndex++;
          }  
        }
      }
      // If we still have the polygon after clipping add this to the list
      int nVerts = vP.Num()-nOffsV;
      if (nVerts >= 3) 
      {
        for (i=0; i<nVerts-2; i++)
        {
          vI.AddElem(nOffsV);
          vI.AddElem(i+nOffsV+1);
          vI.AddElem(i+nOffsV+2);
        }
        
        nCount++; 

        // just in case..
        if( nCount > (float)CRenderer::CV_r_beamsmaxslices )
          break;
      }
    }

    fCurDist += fIncrDist;    
  }

  if (vI.Num() >= 3)
  {
    int nOffs, nIOffs;
    SVF_P3F_C4B_T2F *vDst = (SVF_P3F_C4B_T2F *)rd->GetVBPtr(vP.Num(), nOffs, POOL_P3F_COL4UB_TEX2F);
    uint16 *iDst = rd->GetIBPtr(vI.Num(), nIOffs);

    for (i=0; i<(int)vP.Num(); i++)
    {
      vDst[i].xyz = vP[i];
    }
    memcpy(iDst, &vI[0], vI.Num()*sizeof(uint16));

    rd->UnlockVB(POOL_P3F_COL4UB_TEX2F);
    rd->UnlockIB();

    // Set culling mode
    if (!(rd->m_RP.m_FlagsPerFlush & RBSI_NOCULL))
    {
      if (sl->m_eCull != -1)
        rd->D3DSetCull((ECull)sl->m_eCull);
    }

    if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
      HRESULT h = rd->FX_SetVStream(0, rd->m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
      h = rd->FX_SetIStream(rd->m_pIB);
      rd->FX_Commit();
  #if defined (DIRECT3D9) || defined (OPENGL)
      h = rd->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, nOffs, 0, vP.Num(), nIOffs, vI.Num()/3);
  #elif defined (DIRECT3D10)
      rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      if (rd->m_pd3dDebug)
      {
        h = rd->m_pd3dDebug->ValidateContext(rd->m_pd3dDeviceContext);
        assert(SUCCEEDED(h));
      }
      rd->m_pd3dDeviceContext->DrawIndexed(vI.Num(), nIOffs, nOffs);
  #endif
      rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += vI.Num()/3;
      rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
    }
  }
  else
  {
    rd->FX_Commit();  
  }


  rd->m_RP.m_pCurObject = pObj;
  rd->m_RP.m_pCurInstanceInfo = &rd->m_RP.m_pCurObject->m_II;
  rd->m_RP.m_pShaderResources = pRes;

  if (!CRenderer::CV_r_beamssoftclip)
  {
    if (rd->m_RP.m_ClipPlaneEnabled == 2)
    {
      Plane pl;
      pl.n = rd->m_RP.m_CurClipPlane.m_Normal;
      pl.d = rd->m_RP.m_CurClipPlane.m_Dist;
      Plane plTr = TransformPlane2(rd->m_InvCameraProjMatrix, pl);
#if defined (DIRECT3D9) || defined (OPENGL)
      rd->m_pd3dDevice->SetClipPlane(0, &plTr.n[0]);
      rd->m_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0x1);
#elif defined (DIRECT3D10)
      assert(0);
#endif
    }
    else
    {
#if defined (DIRECT3D9) || defined (OPENGL)
      rd->m_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
#elif defined (DIRECT3D10)
      assert(0);
#endif
    }
  }


  if( bUseOptBeams )
  {
    rd->FX_PopRenderTarget(0);          
    rd->RT_SetViewport(vX, vY, vWidth, vHeight); 

    // Activate glow effect    
    if( !CRenderer::CV_r_HDRRendering)
    {
      CEffectParam *pParam = PostEffectMgr().GetByName("Glow_Active"); 
      assert(pParam && "Parameter doesn't exist");
      pParam->SetParam(1.0f);   
    }
    
    gcpRendD3D->FX_Commit();

    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_LIGHTSHAFTS;
  }

  rd->m_RP.m_FlagsShader_RT = nCurrFlagShader_RT;
  rd->m_RP.m_PrevLMask = -1;
    
  return true;
} 

bool CREParticle::mfPreDraw(SShaderPass *sl)
{
	CD3D9Renderer* rd(gcpRendD3D);
	SRenderPipeline& rp(rd->m_RP);

	bool bGeomShader = rp.m_pCurTechnique && (rp.m_pCurTechnique->m_Flags & FHF_USE_GEOMETRY_SHADER);
	if (rp.m_RendNumVerts && (bGeomShader || rp.m_RendNumIndices))
	{
		uint32 nStart;
		uint32 nSize = rp.m_Stride * rp.m_RendNumVerts;
		if (!(rp.m_FlagsPerFlush & RBSI_VERTSMERGED))
		{
			rp.m_FlagsPerFlush |= RBSI_VERTSMERGED;
			void *pVB = rd->FX_LockVB(nSize, nStart);
#ifndef XENON
			memcpy(pVB, rp.m_Ptr.Ptr, nSize);


#endif
			rd->FX_UnlockVB();
			rp.m_FirstVertex = 0;
			rp.m_MergedStreams[0] = rp.m_VBs[rp.m_CurVB];
			rp.m_nStreamOffset[0] = nStart;
			rp.m_PS[rd->m_RP.m_nProcessThreadID].m_DynMeshUpdateBytes += nSize;
#if defined(DIRECT3D10)
			assert(!bGeomShader && rp.m_RendNumIndices || bGeomShader && !rp.m_RendNumIndices);
			if (rp.m_RendNumIndices)
#endif
			{
				uint16 *pIB = rp.m_IndexBuf->Lock(rp.m_RendNumIndices, nStart);
#ifndef XENON
				memcpy(pIB, rp.m_SysRendIndices, rp.m_RendNumIndices * sizeof(short));


#endif
				rp.m_IndexBuf->Unlock();
				rp.m_FirstIndex = nStart;
				rp.m_PS[rd->m_RP.m_nProcessThreadID].m_DynMeshUpdateBytes += rp.m_RendNumIndices * sizeof(short);
			}
		}
		rp.m_MergedStreams[0].VBPtr_0->Bind(0, rp.m_nStreamOffset[0], rp.m_Stride);		
		rp.m_IndexBuf->Bind();
	}

	return true;
}

bool CREParticle::mfDraw(CShader *ef, SShaderPass *sl)
{
	CD3D9Renderer* rd(gcpRendD3D);
	SRenderPipeline& rp(rd->m_RP);
	
	rd->FX_Commit();

#if defined (DIRECT3D10)
	bool bGeomShader = rp.m_pCurTechnique && (rp.m_pCurTechnique->m_Flags & FHF_USE_GEOMETRY_SHADER);
	if (bGeomShader && CHWShader_D3D::m_pCurInstGS && !CHWShader_D3D::m_pCurInstGS->m_bFallback && CHWShader_D3D::m_pCurInstPS && !CHWShader_D3D::m_pCurInstPS->m_bFallback && CHWShader_D3D::m_pCurInstVS && !CHWShader_D3D::m_pCurInstVS->m_bFallback)
	{
	  if (rp.m_RendNumVerts)
	  {
		  rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		  if (rd->m_pd3dDebug)
			  assert(SUCCEEDED(rd->m_pd3dDebug->ValidateContext(rd->m_pd3dDeviceContext)));
		  rd->m_pd3dDeviceContext->Draw(rp.m_RendNumVerts, 0);

		  rp.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rp.m_nPassGroupDIP] += rp.m_RendNumVerts * 2;
		  ++rp.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rp.m_nPassGroupDIP];
	  }
	}
	else
  if (CHWShader_D3D::m_pCurInstPS && !CHWShader_D3D::m_pCurInstPS->m_bFallback && CHWShader_D3D::m_pCurInstVS && !CHWShader_D3D::m_pCurInstVS->m_bFallback)
#endif
	{
		int numFaces(rd->m_RP.m_RendNumIndices / 3);
		if (numFaces)
		{
#if defined (DIRECT3D9) || defined (OPENGL)
			rd->FX_DebugCheckConsistency(rp.m_FirstVertex, rp.m_FirstIndex, rp.m_RendNumVerts, rp.m_RendNumIndices);
			rd->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, rp.m_FirstVertex, rp.m_RendNumVerts, rp.m_FirstIndex + rp.m_IndexOffset, numFaces);
#elif defined (DIRECT3D10)
			rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			if (rd->m_pd3dDebug)
				assert(SUCCEEDED(rd->m_pd3dDebug->ValidateContext(rd->m_pd3dDeviceContext)));
			rd->m_pd3dDeviceContext->DrawIndexed(rp.m_RendNumIndices, rp.m_FirstIndex + rp.m_IndexOffset, 0);
#endif
			rp.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rp.m_nPassGroupDIP] += numFaces;
			++rp.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rp.m_nPassGroupDIP];
		}
    else
    {
      int nnn = 0;
    }
	}

	return true;
}

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
bool CREParticleGPU::mfDraw( CShader *ef, SShaderPass *sfm )
{
	IGPUPhysicsManager *pGPUManager;

	//pGPUManager = GetSystem()->GetIGPUPhysicsManager();
	pGPUManager = iSystem->GetIGPUPhysicsManager();

	//if no GPU manager, do not try to render the system
	if( pGPUManager == NULL )
	{
		return false;
	}

#if defined (DIRECT3D10)
	assert(0);
#else 
	//render the particle system 
	if( m_nGPUParticleIdx != CPG_NULL_GPU_PARTICLE_SYSTEM_INDEX )
	{
		SGPUParticleRenderShaderParams sParticleRenderParams;


		LPDIRECT3DDEVICE9 pD3DDevice = gcpRendD3D->GetD3DDevice();
		CHWShader_D3D *pHWVertexShader, *pHWPixelShader;

		uint32		nNumLightSources;


		//Since this code block is d3d9 only, class can be promoted
		pHWVertexShader = (CHWShader_D3D *) sfm->m_VShader;
		pHWPixelShader  = (CHWShader_D3D *) sfm->m_PShader;

		//
		CD3D9Renderer* rd( gcpRendD3D );
		rd->FX_Commit();
		//pHWPixelShader->mfBind();
		//pD3DDevice->SetPixelShaderConstantF(0, &(pHWVertexShader->m_CurPSParams[0].x), 31 );

		//pHWPixelShader->
		// CD3D9Renderer* rd( gcpRendD3D );		
		// rd->FX_Commit();


		//The reason the code to setup the rendering parameters is here is because
		// the functions are part of the rendering .dll and and not exposed externally
		// also, some of the classes used have static members, that are not accessible from
		// inside a different .dll


		//From the instance of the current vertex shader, derive the current number of lights 
		// and the light type code from 
		// uint32 uLightMask = pHWVertexShader->m_Insts[ pHWVertexShader->m_CurInst ].m_LightMask;
		uint32 uLightMask = pHWVertexShader->m_pCurInst->m_LightMask;

		//
		nNumLightSources = uLightMask & 0xf;

		// only set this to true if encounter a projected light source
		sParticleRenderParams.m_bHasProj = false;

		//
		if( nNumLightSources == 0 )
		{
			sParticleRenderParams.m_bUsesLights = false;
			sParticleRenderParams.m_nNumLights = nNumLightSources;
		}
		else
		{
			uint32 i;

			sParticleRenderParams.m_bUsesLights = true;
			sParticleRenderParams.m_nNumLights = nNumLightSources;


			//light position for this light source
			SCGParam paramBind;

			//osLightsPos.mfSetOffset( i );
			//osLightsPos.mfGet4f( sParticleRenderParams.m_vOSLightPos[i] );

			//paramBind.m_dwBind = ;
			//paramBind.m_Flags = ;

			// mfSetParameters can be used to retrieve raw parameter data.
			//paramBind.m_eCGParamType = ECGP_PI_OSLightsPos;
			//paramBind.m_eCGParamType = ECGP_PL_LightsPos;
			//paramBind.m_nParameters = nNumLightSources;			//first n lights
			//paramBind.m_pData = NULL;
			//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vOSLightPos[0] ) , pHWVertexShader->m_eSHClass );

			for( i = 0; i < nNumLightSources; i++ )
			{
				int nLightType;
				int nShadOccl;

				//memcpy( sParticleRenderParams.m_vOSLightPos[i], pHWVertexShader->  mfGetMatrixData( &paramBind, nComps ), 16 * sizeof( float ) );

				//light type for this light source
				nLightType = (uLightMask >> (SLMF_LTYPE_SHIFT + i*SLMF_LTYPE_BITS)) & SLMF_TYPE_MASK;

				sParticleRenderParams.m_vLightTypes[i] = nLightType;

				if ( nLightType == SLMF_PROJECTED )
				{
					sParticleRenderParams.m_bHasProj = true;

					// g_mLightMatrix only used when projection is used
					int nComps = 4;  //4 vectors, 
					//SCGParam paramBind;

					// paramBind.m_eCGParamType = ECGP_Matr_LightMatrix;
					// memcpy( sParticleRenderParams.m_mLightMatrix, pHWVertexShader->mfGetMatrixData( &paramBind, nComps ), 16 * sizeof( float ) );

				}

				//not sure how this translates in the new code (no shadows for now..)
				//nShadOccl = ((uLightMask >> (SLMF_LTYPE_SHIFT + i*SLMF_LTYPE_BITS)) & SLMF_SHADOCCLUSION) != 0;

				nShadOccl = 0; 
				if ( nShadOccl )
				{
					sParticleRenderParams.m_bHasShadow = true;
				}
				else
				{
					sParticleRenderParams.m_bHasShadow = false;				
				}

			}

			// not sure where to get this from yet
			sParticleRenderParams.m_fBackLightFraction = 0.0;
		}


		SCGParam paramBind;

		//SParamComp_CameraFront camFront;
		//camFront.mfGet4f( sParticleRenderParams.m_vCamFront );
		paramBind.m_eCGParamType = ECGP_PB_CameraFront;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vCamFront ), pHWVertexShader->m_eSHClass );

		//SParamComp_NearFarDist nearFarDist;
		//nearFarDist.mfGet4f( sParticleRenderParams.m_vNearFarClipDist );
		paramBind.m_eCGParamType = ECGP_PF_NearFarDist;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vNearFarClipDist ), pHWVertexShader->m_eSHClass );

		//SParamComp_SunDirection sunDir;
		//sunDir.mfGet4f( sParticleRenderParams.m_vSunDir );
		paramBind.m_eCGParamType = ECGP_PF_SunDirection;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vSunDir ), pHWVertexShader->m_eSHClass );

		//SParamComp_VolumetricFogParams fogParams;
		//fogParams.mfGet4f( sParticleRenderParams.m_vFogParams );
		paramBind.m_eCGParamType = ECGP_PB_VolumetricFogParams;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vFogParams ), pHWVertexShader->m_eSHClass );

		//SParamComp_VolumetricFogParams fogColor;
		//fogColor.mfGet4f( sParticleRenderParams.m_vFogColor );
		paramBind.m_eCGParamType = ECGP_PB_VolumetricFogColor;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vFogColor ), pHWVertexShader->m_eSHClass );

		//SParamComp_SkyLightHazeColorPartialRayleighInScatter rayleighInScat;
		//rayleighInScat.mfGet4f( sParticleRenderParams.m_vSkyLightHazeColPartialRayleighInScatter );
		paramBind.m_eCGParamType = ECGP_PB_SkyLightHazeColorPartialRayleighInScatter;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vSkyLightHazeColPartialRayleighInScatter ), pHWVertexShader->m_eSHClass );

		//SParamComp_SkyLightHazeColorPartialMieInScatter mieInScat;
		//mieInScat.mfGet4f( sParticleRenderParams.m_vSkyLightHazeColPartialMieInScatter );
		paramBind.m_eCGParamType = ECGP_PB_SkyLightHazeColorPartialMieInScatter;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vSkyLightHazeColPartialMieInScatter ), pHWVertexShader->m_eSHClass );

		//SParamComp_SkyLightSunDirection slSunDir;
		//slSunDir.mfGet4f( sParticleRenderParams.m_vSkyLightSunDirection );
		paramBind.m_eCGParamType = ECGP_PB_SkyLightSunDirection;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vSkyLightSunDirection ), pHWVertexShader->m_eSHClass );

		//SParamComp_SkyLightPhaseFunctionConstants slPhaseFuncConst;
		//slPhaseFuncConst.mfGet4f( sParticleRenderParams.m_vSkyLightPhaseFunctionConstants );
		paramBind.m_eCGParamType = ECGP_PB_SkyLightPhaseFunctionConstants;
		paramBind.m_nParameters = 1;						
		paramBind.m_pData = NULL;
		//pHWVertexShader->mfSetParameters( &paramBind, 1, (float *)&( sParticleRenderParams.m_vSkyLightPhaseFunctionConstants ), pHWVertexShader->m_eSHClass );


		//pass through cshader and 
		pGPUManager->RenderParticleSystem( m_nGPUParticleIdx, ef, sfm, &sParticleRenderParams );


		//restore shader constants.
		//pHWVertexShader->mfCommitParams();

		gcpRendD3D->FX_TagVStreamAsDirty( 0 );
		gcpRendD3D->FX_TagVStreamAsDirty( 1 );
		gcpRendD3D->FX_TagVStreamAsDirty( 2 );
		gcpRendD3D->FX_TagVStreamAsDirty( 3 );
		gcpRendD3D->FX_TagVStreamAsDirty( 4 );
		gcpRendD3D->FX_TagVStreamAsDirty( 5 );
		gcpRendD3D->FX_TagIStreamAsDirty( );
		gcpRendD3D->FX_TagVertexDeclarationAsDirty( );

		pD3DDevice->SetVertexShaderConstantF(0, &(pHWVertexShader->m_CurVSParams[0].x), 30);

		//restore vertex shader
		pHWVertexShader->mfBind();

		//update number of triangles rendered
		//gRenDev->m_nPolygons += pGPUManager->GetNumPolygons( m_nGPUParticleIdx );

	}
#endif

	return true;
}

#endif

bool CREIrradianceVolume::mfDraw(CShader* ef, SShaderPass* sfm)
{
	Evaluate();
	return true;
}

bool CREGameEffect::mfDraw(CShader* ef, SShaderPass* sfm)
{
	CRY_ASSERT_MESSAGE(gRenDev->m_pRT->IsRenderThread(),"Trying to render from wrong thread");
	CRY_ASSERT(ef);
	CRY_ASSERT(sfm);

	if(m_pImpl)
	{
		uint32 passCount	= 0;
		bool successFlag	= true;

		// Begin drawing
		ef->FXBegin(&passCount, 0);
		if(passCount > 0)
		{
			// Begin pass
			ef->FXBeginPass(0);

			// Draw element
			successFlag = m_pImpl->mfDraw(ef,sfm);

			// End pass
			ef->FXEndPass();
		}
		// End drawing
		ef->FXEnd();

		return successFlag;
	}
	return false;
}
