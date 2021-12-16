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
#include <IFlashPlayer.h>

#pragma warning(disable: 4244)

using namespace NPostEffects;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool C3DHud::Preprocess()
{
	const uint32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;
	if( m_pRenderData[nThreadID].empty() )
		return false;

	std::sort(m_pRenderData[nThreadID].begin(), m_pRenderData[nThreadID].end(), HudDataSortCmp());

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
	
void C3DHud::Reset()
{
	m_pRenderData[0].resize(0);
	m_pRenderData[1].resize(0);

	m_pOpacityMul->ResetParam(1.0f);
	m_pHudColor->ResetParamVec4(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	m_pGlowMul->ResetParam(1.0f);
	m_pChromaShift->ResetParam(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::AddRE( const CRendElementBase *re, const SShaderItem *pShaderItem, const CRenderObject *pObj)
{
	// Main thread
	const uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;

	SHudData pHudData(re, pShaderItem, pObj);

	m_pRenderData[nThreadID].push_back( pHudData );

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::OnBeginFrame()
{
	// Main thread
	const uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
	m_pRenderData[nThreadID].resize(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::SetShaderParams( SHudData &pData )
{
	SRenderShaderResources *pShaderResources = (SRenderShaderResources*)pData.pShaderItem->m_pShaderResources;
	float fOpacity = pShaderResources->Opacity() * m_pOpacityMul->GetParam();

	const CRenderObject *pRO = pData.pRO;
	
	Matrix44A mObjCurr;
	mObjCurr.Transpose(pRO->m_II.m_Matrix);   
	
	Matrix44A mViewProj;
	mViewProj.Multiply(mObjCurr, gRenDev->m_CameraProjMatrix); 

	mViewProj.Transpose();

	static CCryName pMatViewProjParamName("mViewProj");
	CShaderMan::m_sh3DHUD->FXSetVSFloat(pMatViewProjParamName, (Vec4 *) mViewProj.GetData(), 4);

	static CCryName pHudTexCoordParamName("HudTexCoordParams");
	float fRcpDstWidth = 1.0f / (float) m_pHUD_RT->GetWidth();
	float fRcpDstHeight = 1.0f / (float) m_pHUD_RT->GetHeight();
	Vec4 vHudTexCoordParams = Vec4(((float) m_nHudTexWidth) * fRcpDstWidth, ((float) m_nHudTexHeight)* fRcpDstHeight, fRcpDstWidth, fRcpDstHeight);
	CShaderMan::m_sh3DHUD->FXSetVSFloat(pHudTexCoordParamName, &vHudTexCoordParams, 1);

	static CCryName pHudParamName("HudParams");
	ColorF cDiffuse = pShaderResources->GetDiffuseColor();
	if (pShaderResources->m_ResFlags & MTL_FLAG_ADDITIVE)
		cDiffuse *= fOpacity;

	Vec4 vHudParams = Vec4(cDiffuse.r, cDiffuse.g, cDiffuse.b, fOpacity) * m_pHudColor->GetParamVec4();  
	CShaderMan::m_sh3DHUD->FXSetPSFloat(pHudParamName, &vHudParams, 1); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::SetTextures( SHudData &pData )
{
	SRenderShaderResources *pShaderResources = (SRenderShaderResources*)pData.pShaderItem->m_pShaderResources;

	SEfResTexture *pDiffuse = (pShaderResources)? pShaderResources->GetTexture( EFTT_DIFFUSE ) : 0;
	if( pDiffuse && pDiffuse->m_Sampler.m_pTex )
		GetUtils().SetTexture(pDiffuse->m_Sampler.m_pTex, 0, FILTER_LINEAR);  
	else
		GetUtils().SetTexture(m_pHUD_RT,  0, FILTER_LINEAR);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::RenderMesh( const CRendElementBase *pRE, SShaderPass *pPass )
{
	CREMesh *pRenderMesh = (CREMesh*) pRE;
	
	// Create/Update vertex buffer stream
	if( pRenderMesh->m_pRenderMesh )
		pRenderMesh->m_pRenderMesh->CheckUpdate( pRenderMesh->m_pRenderMesh->_GetVertexFormat(), 0);

	gRenDev->m_RP.m_pRE = const_cast<CRendElementBase*>( pRE );
	if( gcpRendD3D->FX_CommitStreams(pPass, true) )
	{
		gRenDev->m_RP.m_FirstVertex = pRenderMesh->m_nFirstVertId;
		gRenDev->m_RP.m_FirstIndex = pRenderMesh->m_nFirstIndexId;
		gRenDev->m_RP.m_RendNumIndices = pRenderMesh->m_nNumIndices;
		gRenDev->m_RP.m_RendNumVerts = pRenderMesh->m_nNumVerts;
		gRenDev->m_RP.m_pRE->mfDraw(CShaderMan::m_sh3DHUD, pPass);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::FlashPlayerPreprocess()
{
	PROFILE_LABEL_PUSH( "3D HUD FLASHPLAYER UPDATE" );

	const uint32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;


	uint32 nRECount = m_pRenderData[nThreadID].size();

	// enable rendering outside backbuffer edram range
	m_pHUD_RT->SetRenderTargetTile(1);

	for(int r = 0; r < nRECount; ++r) 
	{
		SHudData &pData = m_pRenderData[nThreadID][r];
		if( !pData.pRO || !pData.pRE || !pData.pShaderItem )
			continue;

		SRenderShaderResources *pShaderResources = (SRenderShaderResources*)pData.pShaderItem->m_pShaderResources;
		if( !pShaderResources ) 
			continue;

		SEfResTexture *pDiffuse = (pShaderResources)? pShaderResources->GetTexture( EFTT_DIFFUSE ) : 0;
		if( pDiffuse && pDiffuse->m_Sampler.m_pDynTexSource )
		{
			pDiffuse->m_Sampler.m_pDynTexSource->Update(m_pHUD_RT);

			IFlashPlayer* pFlashPlayer(0);
			IDynTextureSource::EDynTextureSource type(IDynTextureSource::DTS_I_FLASHPLAYER);
			pDiffuse->m_Sampler.m_pDynTexSource->GetDynTextureSource((void*&)pFlashPlayer, type);
			if (pFlashPlayer && type == IDynTextureSource::DTS_I_FLASHPLAYER)
			{
				m_nHudTexWidth = pFlashPlayer->GetWidth();
				m_nHudTexHeight = pFlashPlayer->GetHeight();
			}
			break;
		}
	}


	m_pHUD_RT->SetRenderTargetTile(0);

	PROFILE_LABEL_POP( "3D HUD FLASHPLAYER UPDATE" );

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::DownsampleHud2x2( CTexture *pDstRT )
{
	PROFILE_LABEL_PUSH( "3D HUD DOWNSAMPLE 2X2" );

	const uint32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;

	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	uint32 nRECount = m_pRenderData[nThreadID].size();

	// enable rendering outside backbuffer edram range
	pDstRT->SetRenderTargetTile(1); 

	// Hud downsampling
	gcpRendD3D->FX_PushRenderTarget(0,  pDstRT,  &gcpRendD3D->m_DepthBufferOrigFSAA); 
	gcpRendD3D->RT_SetViewport(0, 0, pDstRT->GetWidth(), pDstRT->GetHeight());        

	// todo: will likely faster to just render geometry and clearing areas with black
	ColorF clearColor(0, 0, 0, 0.0f);
	gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR, &clearColor); 

	for(int r = 0; r < nRECount; ++r) 
	{
		SHudData &pData = m_pRenderData[nThreadID][r];
		if( !pData.pRO || !pData.pRE || !pData.pShaderItem )
			continue;

		SRenderShaderResources *pShaderResources = (SRenderShaderResources*)pData.pShaderItem->m_pShaderResources;
		if( !pShaderResources || !(pShaderResources->m_ResFlags & MTL_FLAG_ADDITIVE) )
			continue;

		GetUtils().ShBeginPass(CShaderMan::m_sh3DHUD, m_pDownsampleTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);          

		int nRenderState = GS_NODEPTHTEST; 
		gcpRendD3D->EF_SetState( nRenderState );   
		gcpRendD3D->SetCullMode( R_CULL_NONE );

		SetShaderParams( pData );
		SetTextures( pData );
  
		SShaderTechnique *pShaderTech = CShaderMan::m_sh3DHUD->mfFindTechnique(m_pDownsampleTechName);
		SShaderPass *pPass = &pShaderTech->m_Passes[0];

		RenderMesh( pData.pRE, pPass );

		GetUtils().ShEndPass();
	}

	// Restore previous viewport
	gcpRendD3D->FX_PopRenderTarget(0);

	// optimization todo: use hud geometry for culling unnafected areas
	gcpRendD3D->Set2DMode(true, 1, 1);
	GetUtils().TexBlurIterative(pDstRT, 1);           
	gcpRendD3D->Set2DMode(false, 1, 1);

	pDstRT->SetRenderTargetTile(0);

	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);     

	PROFILE_LABEL_POP( "3D HUD DOWNSAMPLE 2X2" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DHud::Render()
{
	const uint32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;

  PROFILE_SHADER_START

	PROFILE_LABEL_PUSH( "3D HUD" );

	m_nTexStateLinearClamp = CTexture::GetTexState( STexState(FILTER_LINEAR, true) );
	m_nTexStateLinearWrap = CTexture::GetTexState( STexState(FILTER_LINEAR, false) );

	// Share hud render target with scene normals
	m_pHUD_RT = CTexture::s_ptexSceneNormalsMap;









	gcpRendD3D->Set2DMode(false, 1, 1);

	CCryName pGeneralTechName("General");

	CRendElementBase *pPrevRE = gRenDev->m_RP.m_pRE;

	uint32 nRECount = m_pRenderData[nThreadID].size();

	// Update flash
	FlashPlayerPreprocess();

	// Downsample hud into half res target - we use this for Bloom/Dof
	DownsampleHud2x2( CTexture::s_ptexBackBufferScaled[1] );

	// hud simple 2D dof blend
	CPostEffect *pDofPostEffect = PostEffectMgr().GetEffect(ePFX_eDepthOfField);
	bool bGameDof = pDofPostEffect->IsActive();

	static float fDofBlend = 0.0f;
	fDofBlend += ((bGameDof?1.0f:0.0f) - fDofBlend) * gEnv->pTimer->GetFrameTime() * 10.0f; 

	for(int r = 0; r < nRECount; ++r) 
	{
		SHudData &pData = m_pRenderData[nThreadID][r];
		if( !pData.pRO || !pData.pRE || !pData.pShaderItem )
			continue;

		SRenderShaderResources *pShaderResources = (SRenderShaderResources*)pData.pShaderItem->m_pShaderResources;
		if( !pShaderResources )
			continue;

		float fGlowAmount = pShaderResources->Glow();
		int nRenderState = GS_NODEPTHTEST; 

		if ((pShaderResources->Opacity()) != 1.0f)
		{
			if (pShaderResources->m_ResFlags & MTL_FLAG_ADDITIVE)
				nRenderState |= GS_BLSRC_ONE | GS_BLDST_ONE;
			else
				nRenderState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
		}

		GetUtils().ShBeginPass(CShaderMan::m_sh3DHUD, pGeneralTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                                   
 
		gcpRendD3D->EF_SetState( nRenderState );   
		gcpRendD3D->SetCullMode( R_CULL_NONE );

		SetShaderParams( pData );

		// Set additional parameters
		static CCryName pHudEffectsParamName("HudEffectParams");
		Vec4 vHudEffectParams = Vec4(fDofBlend, fGlowAmount * m_pGlowMul->GetParam(), 0.0f, m_pChromaShift->GetParam());  
		CShaderMan::m_sh3DHUD->FXSetPSFloat(pHudEffectsParamName, &vHudEffectParams, 1); 

		SetTextures( pData );

		// Set additional textures
		GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[1], 1, FILTER_LINEAR);    

		SShaderTechnique *pShaderTech = CShaderMan::m_sh3DHUD->mfFindTechnique(pGeneralTechName);
		SShaderPass *pPass = &pShaderTech->m_Passes[0];

		RenderMesh( pData.pRE, pPass );

		GetUtils().ShEndPass(); 
	}

	gRenDev->m_RP.m_pRE = pPrevRE;

	gcpRendD3D->Set2DMode(true, 1, 1);
	





	PROFILE_LABEL_POP( "3D HUD" );

  gcpRendD3D->FX_Flush(); 
  PROFILE_SHADER_END  
}

