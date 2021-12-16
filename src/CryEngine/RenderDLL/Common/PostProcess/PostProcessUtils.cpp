/*=============================================================================
PostProcessUtils.cpp : Post processing common utilities
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Tiago Sousa

=============================================================================*/

#include "StdAfx.h"
#include "PostProcessUtils.h"

using namespace NPostEffects;

RECT  SPostEffectsUtils::m_pScreenRect;
ITimer *SPostEffectsUtils::m_pTimer;
int SPostEffectsUtils::m_iFrameCounter=0;
SD3DSurface *SPostEffectsUtils::m_pCurDepthSurface;
CShader *SPostEffectsUtils::m_pCurrShader;
Matrix44 SPostEffectsUtils::m_pView;
Matrix44 SPostEffectsUtils::m_pProj;
Matrix44 SPostEffectsUtils::m_pViewProj;
int SPostEffectsUtils::m_nColorMatrixFrameID;
Matrix44 SPostEffectsUtils::m_pColorMat;
float SPostEffectsUtils::m_fWaterLevel;

Vec3 SPostEffectsUtils::m_vRT = Vec3(0, 0, 0);
Vec3 SPostEffectsUtils::m_vLT = Vec3(0, 0, 0);
Vec3 SPostEffectsUtils::m_vLB = Vec3(0, 0, 0);
Vec3 SPostEffectsUtils::m_vRB = Vec3(0, 0, 0);
int SPostEffectsUtils::m_nFrustrumFrameID = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool SPostEffectsUtils::Create()
{
  assert( gRenDev );

  int iTempX, iTempY, iWidth, iHeight;
  gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
  
  // Update viewport info
  m_pScreenRect.left=iTempX;  
  m_pScreenRect.top=iTempY;                                      

  m_pScreenRect.right=iTempX+iWidth;
  m_pScreenRect.bottom=iTempY+iHeight; 

  if( CRenderer::CV_r_PostMSAA )
	{
    CreateRenderTarget("$PrevBackBuffer", CTexture::s_ptexPrevBackBuffer, m_pScreenRect.right, m_pScreenRect.bottom, 1, 0, eTF_A8R8G8B8, TO_PREVBACKBUFFERMAP);
		CreateRenderTarget("$PrevZTarget", CTexture::s_ptexPrevZTarget, m_pScreenRect.right, m_pScreenRect.bottom, 1, 0, eTF_R32F);
	}
  else
	{
    SAFE_RELEASE(CTexture::s_ptexPrevBackBuffer);
		SAFE_RELEASE(CTexture::s_ptexPrevZTarget);
	}

#if defined (DIRECT3D10) && !defined(PS3)
  // Due to D3D10 srgb limitations on implementation (cannot create typeless swap chain to allow casting between srgb/non-srgb render target view) 
  //we'll need extra render target when using srgb 
  if( gRenDev->IsLinearSpaceShadingEnabled() )
    CreateRenderTarget("$BackBufferTemp", CTexture::s_ptexBackBufferTemp, m_pScreenRect.right, m_pScreenRect.bottom, 1, 0, eTF_A8R8G8B8, -1, FT_USAGE_ALLOWREADSRGB);
#endif

	if( !gRenDev->IsHDRModeEnabled() )
		CreateRenderTarget("$Glow_RT1", CTexture::s_ptexGlow, m_pScreenRect.right>>1, m_pScreenRect.bottom>>1, 1, 0);
	else
		SAFE_RELEASE(CTexture::s_ptexGlow);

  CreateRenderTarget("$BackBufferScaled_d2", CTexture::s_ptexBackBufferScaled[0], m_pScreenRect.right>>1, m_pScreenRect.bottom>>1, 1, 0);  
  CreateRenderTarget("$BackBufferScaled_d4", CTexture::s_ptexBackBufferScaled[1], m_pScreenRect.right>>2, m_pScreenRect.bottom>>2, 1, 0, eTF_A8R8G8B8, TO_BACKBUFFERSCALED_D4);    
	
  // FIXME: Better to use dynamic RT for this case
  CreateRenderTarget("$EffectsAccum", CTexture::s_ptexEffectsAccum, m_pScreenRect.right>>2, m_pScreenRect.bottom>>2, 1, 0);        

  CreateRenderTarget("$WaterPuddlesPrev", CTexture::s_ptexWaterPuddles[0], 256, 256, 1, 0);
  CreateRenderTarget("$WaterPuddlesCurr", CTexture::s_ptexWaterPuddles[1], 256, 256, 1, 0);
  CreateRenderTarget("$WaterPuddlesDDN", CTexture::s_ptexWaterPuddlesDDN, 256, 256, 1, 0, eTF_A8R8G8B8, TO_WATERPUDDLESMAP);
  
  CreateRenderTarget("$BackBufferScaled_d8", CTexture::s_ptexBackBufferScaled[2], m_pScreenRect.right>>3, m_pScreenRect.bottom>>3, 1, 0);        

  CreateRenderTarget("$WaterVolumeDDN", CTexture::s_ptexWaterVolumeDDN, 64, 64, 1, 0, eTF_A8R8G8B8, TO_WATERVOLUMEMAP);        

  return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::Release()
{
  SAFE_RELEASE(CTexture::s_ptexBackBufferTemp);
  SAFE_RELEASE(CTexture::s_ptexPrevBackBuffer);
  SAFE_RELEASE(CTexture::s_ptexEffectsAccum);
  SAFE_RELEASE(CTexture::s_ptexGlow);
  SAFE_RELEASE(CTexture::s_ptexCloak);
  SAFE_RELEASE(CTexture::s_ptexWaterPuddles[0]);
  SAFE_RELEASE(CTexture::s_ptexWaterPuddles[1]);
  SAFE_RELEASE(CTexture::s_ptexWaterPuddlesDDN);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::DrawFullScreenQuad(int nTexWidth, int nTexHeight, uint32 nTessX , uint32 nTessY )
{    
	// No offsets required in d3d10
	float fOffsetU = 0.0f;
	float fOffsetV = 0.0f;

#if defined (DIRECT3D9)

	fOffsetU = 0.5f/(float)nTexWidth;
	fOffsetV = 0.5f/(float)nTexHeight;  

#endif

	if( !nTessX && !nTessY )
	{
		static SVF_P3F_C4B_T2F pScreenQuad[] =
		{
			{ Vec3(0, 0, 0), {{~0}}, Vec2(0, 0) },   
			{ Vec3(0, 0, 0), {{~0}}, Vec2(0, 1) },    
			{ Vec3(0, 0, 0), {{~0}}, Vec2(1, 0) },   
			{ Vec3(0, 0, 0), {{~0}}, Vec2(1, 1) },   
		};

		pScreenQuad[0].xyz = Vec3(-fOffsetU, -fOffsetV, 0);
		pScreenQuad[1].xyz = Vec3(-fOffsetU, 1-fOffsetV, 0);
		pScreenQuad[2].xyz = Vec3(1-fOffsetU, -fOffsetV, 0);
		pScreenQuad[3].xyz = Vec3(1-fOffsetU, 1-fOffsetV, 0);

		CVertexBuffer strip(pScreenQuad, eVF_P3F_C4B_T2F);




		gRenDev->DrawPrimitives(&strip, 4);

	}
	else
	{	
		const uint32 nGridSizeX = max((uint32)2, nTessX);
		const uint32 nGridSizeY = max((uint32)2, nTessY);
		static CVertexBuffer *pVB = NULL;
		static CIndexBuffer *pIB = NULL;

		static PodArray<SVF_P3F_C4B_T2F> pVerts;
		static PodArray<uint16> pIndices;
		if( pVerts.empty() || pIndices.empty() || nGridSizeX * nGridSizeY != pVerts.Count() )
		{
			uint32 nOldVBSize = pVerts.Count();
			uint32 nOldIBSize = pIndices.Count();

			pVerts.Clear();
			pIndices.Clear();
			
			
			float fRcpGridSizeX = 1.0f / (float) (nGridSizeX - 1);			
			float fRcpGridSizeY = 1.0f / (float) (nGridSizeY - 1);			

			SVF_P3F_C4B_T2F tmp;
			Vec3 vv = Vec3(0);
			// Create grid vertices
			for( uint32 y(0); y< nGridSizeY; ++y )
			{      
				vv.y = (float) y * fRcpGridSizeY;// + fRcpGridSize;
				for( uint32 x(0); x< nGridSizeX; ++x )
				{
					vv.x = (float) x * fRcpGridSizeX;// + fRcpGridSize;
					Vec3 vv0 = vv - Vec3(fOffsetU, fOffsetV, 0.0f);
          tmp.xyz = vv0;

					tmp.st = Vec2( vv.x, vv.y );
					pVerts.Add( tmp );        
				}
			}

			// Set vertex buffer
			//pVB.m_VS[VSF_GENERAL].m_VData = &pVerts[0];
			//pVB.m_nVertexFormat = eVF_P3F_C4B_T2F;
			if (pVB)
				gRenDev->m_DevBufMan.ReleaseVBuffer(pVB);
			pVB = gRenDev->m_DevBufMan.CreateVBuffer(pVerts.Count(), eVF_P3F_C4B_T2F, "FS_QUAD");
			gRenDev->m_DevBufMan.UpdateVBuffer(pVB, &pVerts[0], pVerts.Count());

			// Create indices
			int nIndex = 0;
			for( uint32 y(0); y< nGridSizeY - 1; ++y )
			{      
				for( uint32 x(0); x< nGridSizeX; ++x, ++nIndex )
				{        
					pIndices.Add( nIndex );
					pIndices.Add( nIndex + nGridSizeX); 
				}

				if( nGridSizeY - 2 > y)
				{
					pIndices.Add( nIndex + nGridSizeX -1); 
					pIndices.Add( nIndex ); 
				}
			}

			// Set indices
      if (pIB)
        gRenDev->m_DevBufMan.ReleaseIBuffer(pIB);
      pIB = gRenDev->m_DevBufMan.CreateIBuffer(pIndices.Count(), "FS_QUAD");
      gRenDev->m_DevBufMan.UpdateIBuffer(pIB, &pIndices[0], pIndices.Count());
		}		

		assert(pVB && pIB);
		if (!pVB || !pIB)
			return;

		gRenDev->DrawBuffer(pVB, pIB, pIndices.Count(), 0, R_PRIMV_TRIANGLE_STRIP, 0, pVerts.Count());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::DrawScreenQuad(int nTexWidth, int nTexHeight, float x0, float y0, float x1, float y1)
{
  Vec3 vv[4];
  vv[0] = Vec3(x0, y0, 0);
  vv[1] = Vec3(x0, y1, 0);
  vv[2] = Vec3(x1, y0, 0);
  vv[3] = Vec3(x1, y1, 0);
  SVF_P3F_C4B_T2F pScreenQuad[] =  
  {
		{ Vec3(0, 0, 0), {{0}}, Vec2(0, 0) },   
    { Vec3(0, 0, 0), {{0}}, Vec2(0, 1) },    
    { Vec3(0, 0, 0), {{0}}, Vec2(1, 0) },   
    { Vec3(0, 0, 0), {{0}}, Vec2(1, 1) },   
  };

  // No offsets required in d3d10
  float fOffsetU = 0.0f;
  float fOffsetV = 0.0f;

#if defined (DIRECT3D9) || defined (XENON)

  fOffsetU = 0.5f/(float)nTexWidth;
  fOffsetV = 0.5f/(float)nTexHeight;  

#endif

  vv[0] += Vec3(-fOffsetU, -fOffsetV, 0);
  vv[1] += Vec3(-fOffsetU, -fOffsetV, 0);
  vv[2] += Vec3(-fOffsetU, -fOffsetV, 0);
  vv[3] += Vec3(-fOffsetU, -fOffsetV, 0);

  pScreenQuad[0].xyz = vv[0];
  pScreenQuad[1].xyz = vv[1];
  pScreenQuad[2].xyz = vv[2];
  pScreenQuad[3].xyz = vv[3];

  gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_COMMIT_PF);  

  CVertexBuffer strip(pScreenQuad, eVF_P3F_C4B_T2F); 

	gRenDev->DrawPrimitives(&strip, 4);


//  gRenDev->DrawPrimitives(&strip, 4);     
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::DrawFullScreenQuadWPOS(int nTexWidth, int nTexHeight)
{    
	UpdateFrustrumCorners();

  // No offsets required in d3d10
  float fOffsetU = 0.0f;
  float fOffsetV = 0.0f;
#if defined (DIRECT3D9) || defined (XENON)
  fOffsetU = 0.5f/(float)nTexWidth;
  fOffsetV = 0.5f/(float)nTexHeight;  
#endif

  SVF_P3F_T2F_T3F pScreenQuad[] =
  {
    { Vec3(-fOffsetU, -fOffsetV, 0), Vec2(0, 0),		m_vLT },   
    { Vec3(-fOffsetU, 1-fOffsetV, 0), Vec2(0, 1), 	m_vLB },    
    { Vec3(1-fOffsetU, -fOffsetV, 0), Vec2(1, 0), 	m_vRT },   
    { Vec3(1-fOffsetU, 1-fOffsetV, 0), Vec2(1, 1),	m_vRB },   
  };

  CVertexBuffer strip(pScreenQuad, eVF_P3F_T2F_T3F);
  gRenDev->DrawPrimitives(&strip, 4);     
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::DrawScreenQuadWPOS(float x, float y, float sx, float sy, float z, int nTexWidth, int nTexHeight)
{    
//  float fInvW = 1.0f / (float) nTexWidth, fInvH = 1.0f / (float) nTexHeight;
	UpdateFrustrumCorners();

  // No offsets required in d3d10
  float fOffsetU = 0.0f;
  float fOffsetV = 0.0f;
#if defined (DIRECT3D9) || defined (XENON)
  fOffsetU = 0.5f/(float)nTexWidth;
  fOffsetV = 0.5f/(float)nTexHeight;  
#endif

  //x = 0;
  //y = 0;
  //sx = nTexWidth; 
  //sy = nTexHeight;

  float fu1 = ((float) x / (float)nTexWidth);
  float fv1 = ((float) y / (float)nTexHeight);
  float fu2 = ((float) (x+sx) / (float)nTexWidth);
  float fv2 = ((float) (y+sy) / (float)nTexHeight);

  float fx1 = -fOffsetU + fu1;
  float fy1 = -fOffsetV + fv1;
  float fx2 = -fOffsetU + fu2;
  float fy2 = -fOffsetV + fv2;

  SVF_P3F_T2F_T3F pScreenQuad[] =
  {
    { Vec3(fx1, fy1, z), Vec2(fu1, fv1), m_vLT },   
    { Vec3(fx1, fy2, z), Vec2(fu1, fv2), m_vLB },    
    { Vec3(fx2, fy1, z), Vec2(fu2, fv1), m_vRT },   
    { Vec3(fx2, fy2, z), Vec2(fu2, fv2), m_vRB },      
  };

  CVertexBuffer strip(pScreenQuad, eVF_P3F_T2F_T3F);

	gRenDev->DrawPrimitives(&strip, 4);  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::SetTexture(CTexture *pTex, int nStage, int nFilter, int nClamp, bool bSRGBLookup)
{
  int nTexState;

  if (pTex)
  {
    STexState TS;
    TS.SetFilterMode(nFilter);        
    TS.SetClampMode(nClamp, nClamp, nClamp);
    if( nClamp == TADDR_BORDER )
      TS.SetBorderColor(0);
    TS.m_bSRGBLookup = bSRGBLookup;
    nTexState = CTexture::GetTexState(TS);
    pTex->Apply(nStage, nTexState); 
  }
  else
  {
    CTexture::ApplyForID(0);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool SPostEffectsUtils::CreateRenderTarget(const char *szTexName, CTexture *&pTex, int iWidth, int iHeight, bool bUseAlpha, bool bMipMaps, ETEX_Format eTF, int nCustomID, int nFlags )
{
  // check if parameters are valid
  if(!iWidth || !iHeight)
  {
    return 0;
  }

  if(pTex)
  {
    pTex->Invalidate(iWidth, iHeight, eTF);

    if(bMipMaps)
    {
      pTex->ResetFlags(FT_NOMIPS);
    }
  }

  uint32 flags = nFlags;
	flags |= FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RESIZE | FT_NOMIPS;

  // if texture doesn't exist yet, create it
  if (!CTexture::IsTextureExist(pTex))
  {
    pTex = CTexture::CreateRenderTarget(szTexName, iWidth, iHeight, eTT_2D, flags , eTF, nCustomID, -1);      
    if(pTex)
    { 
      // Clear render target surface before using it
      ColorF c = ColorF(0, 0, 0, 1);
      pTex->Fill(c);
    }
  }

  return CTexture::IsTextureExist(pTex) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::ShBeginPass( CShader *pShader, const CCryName& TechName, uint32 nFlags )
{
  assert( pShader );

  m_pCurrShader = pShader;

  uint32 nPasses;
  m_pCurrShader->FXSetTechnique(TechName);
  m_pCurrShader->FXBegin(&nPasses, nFlags);
  m_pCurrShader->FXBeginPass(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::ShEndPass( )
{
  assert( m_pCurrShader );

  m_pCurrShader->FXEndPass();
  m_pCurrShader->FXEnd(); 

  m_pCurrShader = 0;
}  

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::ShSetParamVS( const CCryName& pParamName, const Vec4 &pParam )
{
  assert( m_pCurrShader );
  m_pCurrShader->FXSetVSFloat(pParamName, &pParam, 1); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::ShSetParamPS( const CCryName& pParamName, const Vec4 &pParam )
{
  assert( m_pCurrShader );   
  m_pCurrShader->FXSetPSFloat(pParamName, &pParam, 1); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SPostEffectsUtils::ClearScreen(float r, float g, float b, float a)
{  
  static CCryName pTechName("ClearScreen");
  ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

  int iTempX, iTempY, iWidth, iHeight;
  gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  Vec4 pClrScrParms=Vec4(r, g, b, a); 
  static CCryName pParamName("clrScrParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pClrScrParms, 1);        

  DrawFullScreenQuad(iWidth, iHeight);

  ShEndPass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void NPostEffects::SPostEffectsUtils::UpdateFrustrumCorners()
{
  int nFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameID;
	if( m_nFrustrumFrameID != nFrameID )
	{
		Vec3 vCoords[8];
		gRenDev->GetRCamera().CalcVerts( vCoords);

		m_vRT = vCoords[4] - vCoords[0];
		m_vLT = vCoords[5] - vCoords[1];
		m_vLB = vCoords[6] - vCoords[2];
		m_vRB = vCoords[7] - vCoords[3];

		m_nFrustrumFrameID = nFrameID;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

Matrix44 &SPostEffectsUtils::GetColorMatrix()
{
  int nFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameID;
  if( m_nColorMatrixFrameID != nFrameID )
  {
    // Create color transformation matrices
    float fBrightness = PostEffectMgr().GetByNameF("Global_Brightness");
    float fContrast = PostEffectMgr().GetByNameF("Global_Contrast");
    float fSaturation = PostEffectMgr().GetByNameF("Global_Saturation");
    float fColorC = PostEffectMgr().GetByNameF("Global_ColorC");
    float fColorM = PostEffectMgr().GetByNameF("Global_ColorM");
    float fColorY = PostEffectMgr().GetByNameF("Global_ColorY");
    float fColorK = PostEffectMgr().GetByNameF("Global_ColorK");
    float fColorHue = PostEffectMgr().GetByNameF("Global_ColorHue");

    float fUserCyan = PostEffectMgr().GetByNameF("Global_User_ColorC");
    fColorC = fUserCyan;

    float fUserMagenta = PostEffectMgr().GetByNameF("Global_User_ColorM");
    fColorM = fUserMagenta;

    float fUserYellow = PostEffectMgr().GetByNameF("Global_User_ColorY");
    fColorY = fUserYellow;

    float fUserLuminance = PostEffectMgr().GetByNameF("Global_User_ColorK");
    fColorK = fUserLuminance;

    float fUserHue = PostEffectMgr().GetByNameF("Global_User_ColorHue");
    fColorHue = fUserHue;

    float fUserBrightness = PostEffectMgr().GetByNameF("Global_User_Brightness");
    fBrightness = fUserBrightness;

    float fUserContrast = PostEffectMgr().GetByNameF("Global_User_Contrast");
    fContrast = fUserContrast;

    float fUserSaturation = PostEffectMgr().GetByNameF("Global_User_Saturation"); // translate to 0  
    fSaturation = fUserSaturation;

    // Saturation matrix
    Matrix44 pSaturationMat;      

    {
      float y=0.3086f, u=0.6094f, v=0.0820f, s=clamp_tpl<float>(fSaturation, -1.0f, 100.0f);  

      float a = (1.0f-s)*y + s;
      float b = (1.0f-s)*y;
      float c = (1.0f-s)*y;
      float d = (1.0f-s)*u;
      float e = (1.0f-s)*u + s;
      float f = (1.0f-s)*u;
      float g = (1.0f-s)*v;
      float h = (1.0f-s)*v;
      float i = (1.0f-s)*v + s;

      pSaturationMat.SetIdentity();
      pSaturationMat.SetRow(0, Vec3(a, d, g));  
      pSaturationMat.SetRow(1, Vec3(b, e, h));
      pSaturationMat.SetRow(2, Vec3(c, f, i));                                     
    }

    // Create Brightness matrix
    Matrix44 pBrightMat;
    fBrightness=clamp_tpl<float>(fBrightness, 0.0f, 100.0f);    

    pBrightMat.SetIdentity();
    pBrightMat.SetRow(0, Vec3(fBrightness, 0, 0)); 
    pBrightMat.SetRow(1, Vec3(0, fBrightness, 0));
    pBrightMat.SetRow(2, Vec3(0, 0, fBrightness));            

    // Create Contrast matrix
    Matrix44 pContrastMat;
    {
      float c=clamp_tpl<float>(fContrast, -1.0f, 100.0f);  

      pContrastMat.SetIdentity();
      pContrastMat.SetRow(0, Vec3(c, 0, 0));  
      pContrastMat.SetRow(1, Vec3(0, c, 0));
      pContrastMat.SetRow(2, Vec3(0, 0, c));              
      pContrastMat.SetColumn(3, 0.5f*Vec3(1.0f-c, 1.0f-c, 1.0f-c));  
    }

    // Create CMKY matrix
    Matrix44 pCMKYMat;
    {    
      Vec4 pCMYKParams = Vec4(fColorC + fColorK, fColorM + fColorK, fColorY + fColorK, 1.0f); 

      pCMKYMat.SetIdentity();
      pCMKYMat.SetColumn(3, -Vec3(pCMYKParams.x, pCMYKParams.y, pCMYKParams.z));  
    }

    // Create Hue rotation matrix
    Matrix44 pHueMat;
    {    
      pHueMat.SetIdentity();    

      const Vec3 pHueVec = Vec3(0.57735026f, 0.57735026f, 0.57735026f); // (normalized(1,1,1)
      pHueMat = Matrix34::CreateRotationAA(fColorHue * PI, pHueVec);         
      pHueMat.SetColumn(3, Vec3(0, 0, 0));  
    }

    // Compose final color matrix and set fragment program constants
    m_pColorMat = pSaturationMat * (pBrightMat * pContrastMat * pCMKYMat * pHueMat);      

    m_nColorMatrixFrameID = nFrameID;
  }


  return m_pColorMat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int SScreenParticles::m_nFrameID;
Vec3 SScreenParticles::m_pCamVelocity;
Vec3 SScreenParticles::m_pGravity;
Matrix44 SScreenParticles::m_pPrevView;
Matrix44 SScreenParticles::m_pViewProjPrev;

void SScreenParticles::Create(const SScreenParticle &pDefaultSetup, int nCount, bool bUseCamVelocity)
{
  m_pDefaultSettings = pDefaultSetup;
  m_bUseCamVelocity = bUseCamVelocity;
  
  if( !m_pParticles.empty() && m_pParticles.size() == nCount)
  {
    // Already generated, no need to proceed
    return;
  }

  Release();

  m_pParticles.reserve( nCount );
  for( int p( 0 ); p< nCount; ++p )
  {
    SScreenParticle *pParticle = new SScreenParticle;
    (*pParticle) = pDefaultSetup;
    m_pParticles.push_back( pParticle );
  }

  return;
}
  
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SScreenParticles::Release()
{  
  if(m_pParticles.empty())
  {
    return;
  }

  SScreenParticleItor pItor, pItorEnd = m_pParticles.end(); 
  for(pItor = m_pParticles.begin(); pItor != pItorEnd; ++pItor )
  {
    SAFE_DELETE((*pItor));
  } 

  m_pParticles.clear(); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

inline float srandf()
{
  return Random() *  2.0f - 1.0f ;
}

void SScreenParticles::SpawnParticle( SScreenParticle *&pParticle )
{
  pParticle->m_pPos.x = SPostEffectsUtils::randf();
  pParticle->m_pPos.y = SPostEffectsUtils::randf();

  pParticle->m_pPosStart = pParticle->m_pPos;
  pParticle->m_pPrevPos = pParticle->m_pPos;

  pParticle->m_fLifeTime = m_pDefaultSettings.m_fLifeTime + m_pDefaultSettings.m_fLifeTimeVar * SPostEffectsUtils::srandf();

  pParticle->m_fLifeTime *= 0.5f;

  pParticle->m_fSize = m_pDefaultSettings.m_fSize + m_pDefaultSettings.m_fSizeVar * SPostEffectsUtils::srandf();
  pParticle->m_fSize*= 4;

  pParticle->m_fSpawnTime = SPostEffectsUtils::m_pTimer->GetCurrTime();  
  pParticle->m_fWeight = m_pDefaultSettings.m_fWeight + m_pDefaultSettings.m_fWeightVar * SPostEffectsUtils::srandf();

  // test
  m_pDefaultSettings.m_pVelocity = Vec3(0.1f, -0.1f, 0);
  m_pDefaultSettings.m_fVelocityVar = 0.05f;
  
  pParticle->m_pVelocity = m_pDefaultSettings.m_pVelocity +  m_pDefaultSettings.m_fVelocityVar * Vec3( SPostEffectsUtils::srandf(), SPostEffectsUtils::srandf(), 0 ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SScreenParticles::SetShaderTechnique(CShader *pShader, const char *pszTechnique)
{
  assert( pShader && pszTechnique );

  m_pShader = pShader;
  m_pszTechnique = pszTechnique;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

const Vec3 SScreenParticles::GetCamVelocity()
{
  // Update screen force
  if( m_nFrameID != gRenDev->GetFrameID() )
  {
    CRenderCamera pCam = gRenDev->GetRCamera();

    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    Matrix44 pCurrView( SPostEffectsUtils::m_pView ), pCurrProj( SPostEffectsUtils::m_pProj );  

    Matrix33 pLerpedView;
    pLerpedView.SetIdentity(); 

    float fCurrFrameTime = SPostEffectsUtils::m_pTimer->GetFrameTime(); 
    // scale down speed a bit
    float fAlpha = 0.0005f/( fCurrFrameTime);     

    // Interpolate matrixes and position
    pLerpedView = Matrix33(pCurrView)*(1-fAlpha) + Matrix33(m_pPrevView)*fAlpha;   

    Vec3 pLerpedPos =Vec3::CreateLerp(pCurrView.GetRow(3), m_pPrevView.GetRow(3), fAlpha);     

    // Compose final 'previous' viewProjection matrix
    Matrix44 pLView = pLerpedView;
    pLView.m30 = pLerpedPos.x;   
    pLView.m31 = pLerpedPos.y;
    pLView.m32 = pLerpedPos.z; 

    //m_pViewProjPrev = pLView * pCurrProj;      
    m_pViewProjPrev = m_pPrevView * pCurrProj;          
    m_pViewProjPrev.Transpose();

    // Compute camera velocity vector
    Vec3 vz = pCam.Z;    // front vec
    Vec3 vMoveDir = pCam.Orig - vz;
    Vec4 vCurrPos = Vec4(vMoveDir.x, vMoveDir.y, vMoveDir.z, 1.0f);

    Matrix44 pViewProjCurr( SPostEffectsUtils::m_pViewProj );      

    // Compute camera velocity in screen space
    Vec4 pProjCurrPos = pViewProjCurr * vCurrPos;

    pProjCurrPos.x = ( (pProjCurrPos.x + pProjCurrPos.w) * 0.5f + (1.0f / (float) iWidth) * pProjCurrPos.w) / pProjCurrPos.w;
    pProjCurrPos.y = ( (pProjCurrPos.w - pProjCurrPos.y) * 0.5f + (1.0f / (float) iHeight) * pProjCurrPos.w) / pProjCurrPos.w;

    Vec4 pProjPrevPos = m_pViewProjPrev * vCurrPos;

    pProjPrevPos.x = ( (pProjPrevPos.x + pProjPrevPos.w) * 0.5f + (1.0f / (float) iWidth) * pProjPrevPos.w) / pProjPrevPos.w;
    pProjPrevPos.y = ( (pProjPrevPos.w - pProjPrevPos.y) * 0.5f + (1.0f / (float) iHeight) * pProjPrevPos.w) / pProjPrevPos.w;

    m_pCamVelocity = Vec3(pProjCurrPos.x - pProjPrevPos.x, pProjCurrPos.y - pProjPrevPos.y, 0);
  }

  return m_pCamVelocity * SPostEffectsUtils::m_pTimer->GetFrameTime();
}

const Vec3 SScreenParticles::GetGravity()
{
  if( m_nFrameID != gRenDev->GetFrameID() )
  {
    CRenderCamera pCam = gRenDev->GetRCamera();
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    // front vec
    Vec3 vz = pCam.Z;    

    m_pGravity =  Vec3( 0.0f, 0.0f, -0.098f);
    Matrix44 pViewProjCurr( SPostEffectsUtils::m_pViewProj );      

    Vec3 vMoveDir = pCam.Orig - vz;// - m_pGravity;
    Vec4 vCurrPos = Vec4(vMoveDir.x, vMoveDir.y, vMoveDir.z, 1.0f);

    // Compute camera velocity in screen space
    Vec4 pProjCurrPos = pViewProjCurr * vCurrPos;

    pProjCurrPos.x = ( (pProjCurrPos.x + pProjCurrPos.w) * 0.5f + (1.0f / (float) iWidth) * pProjCurrPos.w) / pProjCurrPos.w;
    pProjCurrPos.y = ( (pProjCurrPos.w - pProjCurrPos.y) * 0.5f + (1.0f / (float) iHeight) * pProjCurrPos.w) / pProjCurrPos.w;

    vMoveDir = pCam.Orig - vz - m_pGravity;
    vCurrPos = Vec4(vMoveDir.x, vMoveDir.y, vMoveDir.z, 1.0f);

    Vec4 pProjPrevPos = pViewProjCurr * vCurrPos;

    pProjPrevPos.x = ( (pProjPrevPos.x + pProjPrevPos.w) * 0.5f + (1.0f / (float) iWidth) * pProjPrevPos.w) / pProjPrevPos.w;
    pProjPrevPos.y = ( (pProjPrevPos.w - pProjPrevPos.y) * 0.5f + (1.0f / (float) iHeight) * pProjPrevPos.w) / pProjPrevPos.w;

    m_pGravity = Vec3(pProjCurrPos.x - pProjPrevPos.x, pProjCurrPos.y - pProjPrevPos.y, 0)* SPostEffectsUtils::m_pTimer->GetFrameTime();
  }

  return m_pGravity;
}

void SScreenParticles::ParticleUpdate( SScreenParticle *&pParticle)
{
  float fCurrLifeTime = (SPostEffectsUtils::m_pTimer->GetCurrTime() - pParticle->m_fSpawnTime) / pParticle->m_fLifeTime;

  // Particle died, spawn new 
  if( fabs(fCurrLifeTime) > 1.0f || pParticle->m_fSize < 0.01f)
  {
    SpawnParticle( pParticle );
  }

  pParticle->m_pVelocity += m_pSumVelocity;
  pParticle->m_pVelocity *= (pParticle->m_fWeight * 0.75f);

  // update position - limit position update to half particle size
  pParticle->m_pPos += pParticle->m_pVelocity * min( pParticle->m_fWeight, 0.5f * pParticle->m_fSize);      
}
 
void SScreenParticles::Update()
{  
  // Update camera velocity and gravity
  m_pSumVelocity += GetCamVelocity();
  m_pSumVelocity += GetGravity();

  // Update particle system

  SScreenParticleItor pItor, pItorEnd = m_pParticles.end(); 

  float fCurrFrameTime = SPostEffectsUtils::m_pTimer->GetFrameTime();

  for(pItor=m_pParticles.begin(); pItor!=pItorEnd; ++pItor )
  {
    SScreenParticle *pCurr = (*pItor);

    // update position, etc
    ParticleUpdate( pCurr );
  }

  // Set current frame ID
  m_nFrameID = gRenDev->GetFrameID();
  m_pPrevView = SPostEffectsUtils::m_pView;

  m_pSumVelocity = Vec3(0.0f, 0.0f, 0.0f);
  
  //std::sort(m_pParticles.begin(), m_pParticles.end(), SScreenParticles::ParticleSpawntimeComparator()); 
  //m_pParticles.sort();
}

void SScreenParticles::Render()
{
  assert(m_pShader && m_pszTechnique);

  int iTempX, iTempY, iWidth, iHeight;
  gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  float fWidth = (float) iWidth;
  float fHeight = (float) iHeight;

  gRenDev->Set2DMode(false, 1, 1);       
  gRenDev->Set2DMode(true, iWidth, iHeight);        

  // FIXME: use single draw call

  static CCryName pTechName(m_pszTechnique);
  static CCryName pParamName("vScrParticleInfo");

  SPostEffectsUtils::ShBeginPass(m_pShader, pTechName);   

  SScreenParticleItor pItor, pItorEnd = m_pParticles.end(); 
  for(pItor = m_pParticles.begin(); pItor != pItorEnd; ++pItor )
  {
    SScreenParticle *pCurr = (*pItor);

    // render a sprite    
    float x0 = pCurr->m_pPos.x * fWidth;
    float y0 = pCurr->m_pPos.y * fHeight;  

    float x1 = (pCurr->m_pPos.x + 0.01f*pCurr->m_fSize * (fHeight/fWidth)) * fWidth ;
    float y1 = (pCurr->m_pPos.y +  0.01f*pCurr->m_fSize) * fHeight ;          

    float fCurrLifeTime = (SPostEffectsUtils::m_pTimer->GetCurrTime() - pCurr->m_fSpawnTime) / pCurr->m_fLifeTime;

    // x, y, z - unused, w - lifetime
    SPostEffectsUtils::ShSetParamPS( pParamName, Vec4(1.0f, 1.0f, 1.0f, 1.0f - fCurrLifeTime) );

    SPostEffectsUtils::DrawScreenQuad(iWidth, iHeight, x0, y0, x1, y1);
  }

  SPostEffectsUtils::ShEndPass();   

  gRenDev->Set2DMode(false, iWidth, iHeight);      
  gRenDev->Set2DMode(true, 1, 1);         
}
