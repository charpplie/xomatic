/*=============================================================================
  D3DHDRRender.cpp : Direct3D specific high dynamic range post-processing
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

    Revision history:
      * Created by Honich Andrey
    
=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"

using namespace NPostEffects;

//=============================================================================

//----------------------------------------------------------------------------------------
// Render Target Management functions
//   Allocation order affects performance.  These functions try to find an optimal allocation
//   order for an arbitrary number of render targets.
//-----------------------------------------------------------------------------------------

//  defer allocation of render targets until we have a list of all required targets...
//  then, sort the list to make "optimal" use of GPU memory
struct DeferredRenderTarget
{
  DWORD               dwWidth;
  DWORD               dwHeight;
  ETEX_Format         Format;
  DWORD               dwFlags;
  CTexture            **lplpStorage;
  char                szName[64];
  DWORD               dwPitch;
  float               fPriority;
  int32               nCustomID;
	bool								bHiQuality;
};

std::vector< DeferredRenderTarget* > g_vAllRenderTargets;
#define SORT_RENDERTARGETS TRUE

struct DescendingRenderTargetSort
{
  bool operator()( DeferredRenderTarget* drtStart, DeferredRenderTarget *drtEnd ) { return (drtStart->dwPitch*drtStart->fPriority) > (drtEnd->dwPitch*drtEnd->fPriority); }
};

//  This function just clears the vector
void StartRenderTargetList( )
{
  std::vector< DeferredRenderTarget* >::iterator _it = g_vAllRenderTargets.begin();

  while ( _it != g_vAllRenderTargets.end() )
  {
    DeferredRenderTarget *drt = (DeferredRenderTarget*)*_it++;
    delete drt;
  }

  g_vAllRenderTargets.clear();
}


void SetHDRParams(CShader *pSH)
{
	CD3D9Renderer *r = gcpRendD3D;
	static CCryName Param1Name("HDRParams0");
	static CCryName Param2Name("HDRParams1");
	static CCryName Param3Name("HDRParams2");

	Vec4 v;

	// To maximize precision on bloom we keep original re-scale
	v[0] = CRenderer::CV_r_HDREyeAdaptationFactor;
	v[1] = CRenderer::CV_r_HDRBrightOffset;
  // apply gamma correction to bright offset
  if (r->IsLinearSpaceShadingEnabled())
    v[1] *= v[1];

	v[2] = CRenderer::CV_r_HDRBrightThreshold;
	v[3] = CRenderer::CV_r_HDRBrightLevel;
	pSH->FXSetPSFloat(Param1Name, &v, 1);

	v[0] = CRenderer::CV_r_HDREyeAdaptationBase;
	v[1] = CRenderer::CV_r_HDRLevel; 
	v[2] = CRenderer::CV_r_HDROffset; 
	
	// Output eye adaptation into register
	v[3] = CRenderer::CV_r_HDREyeAdaptationBase * (1.0f - CRenderer::CV_r_HDREyeAdaptationFactor) + CRenderer::CV_r_HDREyeAdaptationFactor * r->m_vSceneLuminanceInfo.x;

	// Output computed adapted luminance, instead of computing in pixel shader ( half fAdaptedLumDest = HDRParams1.y / (1e-6 + 1.0h + HDRParams1.z * fAdaptedLum) )
	if( CRenderer:: CV_r_HDRRangeAdapt == 1) 	
		v[1] = v[1] / (1e-6f + 1.0f + v[2] * v[3] ); 

	if( CRenderer:: CV_r_HDRRangeAdapt == 2) 	
		v[1] = v[1] / (1e-6f + 1.0f + v[2] * v[3] * (1.0f / r->m_fAdaptedSceneScale) ); 


	pSH->FXSetPSFloat(Param2Name, &v, 1);

	v[0] = r->m_vSceneLuminanceInfo.x;
	v[1] = 1.0f / r->m_fAdaptedSceneScale; 
	v[2] = min( CRenderer::CV_r_HDRContrast, 5.0f);
	
	v[3] = CRenderer::CV_r_HDRContrastLuminanceBlend;
	pSH->FXSetPSFloat(Param3Name, &v, 1);
}

//  Add a render target to the list
void AllocateDeferredRenderTarget( DWORD dwWidth, DWORD dwHeight, ETEX_Format Format, float fPriority, const char *szName, CTexture **pStorage, DWORD dwFlags=0, int32 nCustomID = -1, bool bHiQuality=0, bool bDynamicTex = 0)
{
  DeferredRenderTarget *drt = new DeferredRenderTarget;
  drt->dwWidth          = dwWidth;
  drt->dwHeight         = dwHeight;
  drt->dwFlags          = FT_USAGE_RENDERTARGET | FT_DONT_STREAM | FT_DONT_RELEASE | FT_DONT_RESIZE | dwFlags;
  drt->Format           = Format;
	drt->bHiQuality				= bHiQuality;
  drt->fPriority        = fPriority;
  drt->lplpStorage      = pStorage;
  strcpy_s(drt->szName, szName);
  drt->dwPitch          = dwWidth * CTexture::ComponentsForFormat(Format);
  g_vAllRenderTargets.push_back( drt );
}

//  Now, sort and allocate all render targets
bool EndRenderTargetList( )
{
  if ( SORT_RENDERTARGETS )
    std::sort( g_vAllRenderTargets.begin(), g_vAllRenderTargets.end(), DescendingRenderTargetSort() );

  std::vector< DeferredRenderTarget* >::iterator _it = g_vAllRenderTargets.begin();
  bool bRes = true;

  while ( _it != g_vAllRenderTargets.end() )
  {
    DeferredRenderTarget *drt = (DeferredRenderTarget*)*_it++;
    CTexture *tp = CTexture::CreateRenderTarget(drt->szName, drt->dwWidth, drt->dwHeight, eTT_2D, drt->dwFlags, drt->Format, drt->nCustomID, (int8)(drt->fPriority*100.0f)-5, drt->bHiQuality);

    if (tp)
    {
      *drt->lplpStorage = tp;
      ColorF c = Col_Black;
      tp->Fill(c);
    }
    else
      bRes = false;
  }
  StartRenderTargetList( );
  return S_OK;
}

static STexState sTexStateLinear = STexState(FILTER_LINEAR, true);
static STexState sTexStateLinearWrap = STexState(FILTER_LINEAR, false);
static STexState sTexStatePoint = STexState(FILTER_POINT, true);
static STexState sTexStatePointWrap = STexState(FILTER_POINT, false);
static int nTexStateLinear;
static int nTexStateLinearWrap;
static int nTexStatePoint;
static int nTexStatePointWrap;

void CTexture::DestroyHDRMaps()
{
  CD3D9Renderer *r = gcpRendD3D;
  int i;

  SAFE_RELEASE(s_ptexHDRTarget);
#if PS3
  SAFE_RELEASE(s_ptexHDRTargetEncoded);
#endif
  SAFE_RELEASE(s_ptexHDRTargetScaled[0]);
	SAFE_RELEASE(s_ptexCurLumTextureSys);

  for (i=0; i<8; i++)
  {
    SAFE_RELEASE(s_ptexHDRAdaptedLuminanceCur[i]);
  }

  for(i=0; i<NUM_HDR_TONEMAP_TEXTURES; i++)
  {
    SAFE_RELEASE(s_ptexHDRToneMaps[i]);
  }

  CTexture::s_ptexCurLumTexture = NULL;
}

void CTexture::GenerateHDRMaps()
{
  int i;
  char szName[256];

  CD3D9Renderer *r = gcpRendD3D;
	r->m_dwHDRCropWidth = r->GetWidth();
  r->m_dwHDRCropHeight = r->GetHeight();

  DestroyHDRMaps();

  StartRenderTargetList();

	ETEX_Format nHDRFormat = eTF_A16B16G16R16F;






  if (r->m_nHDRType > 1)
    AllocateDeferredRenderTarget(r->GetWidth(), r->GetHeight(), eTF_A8R8G8B8, 1.0f, "$HDRTarget", &s_ptexHDRTarget);
  else
  {
		AllocateDeferredRenderTarget(r->GetWidth(), r->GetHeight(), nHDRFormat, 1.0f, "$HDRTarget", &s_ptexHDRTarget, ((CRenderer::CV_r_fsaa ? FT_USAGE_FSAA : 0) ) );
#if defined( PS3 )
    AllocateDeferredRenderTarget(r->GetWidth(), r->GetHeight(), eTF_A8R8G8B8, 1.0f, "$HDRTargetEncoded", &s_ptexHDRTargetEncoded, (CRenderer::CV_r_fsaa ? FT_USAGE_FSAA : 0) | FT_USAGE_PREDICATED_TILING/*| FT_CUSTOM_FORMAT*/, TO_HDRTARGET_ENCODED);
#endif
  }






	// Scaled versions of the HDR scene texture
  AllocateDeferredRenderTarget((r->m_dwHDRCropWidth>>1), (r->m_dwHDRCropHeight>>1), /*eTF_A16B16G16R16F*/nHDRFormat ,0.9f, "$HDRTargetScaled0", &s_ptexHDRTargetScaled[0], (!CRenderer::CV_r_HDRTexFormat?FT_CUSTOM_FORMAT:0));
	
  for (i=0; i<8; i++)
  {
    sprintf(szName, "$HDRAdaptedLuminanceCur_%d", i);
    AllocateDeferredRenderTarget(1, 1, /*r->m_HDR_FloatFormat_Scalar/ **/eTF_G16R16F, 0.1f, szName, &s_ptexHDRAdaptedLuminanceCur[i], 0, -1, true);
  }

	DWORD dwFlagsDyn = FT_DONT_STREAM | FT_DONT_RELEASE | FT_DONT_RESIZE | FT_USAGE_READBACK;//*/FT_USAGE_RENDERTARGET;
	s_ptexCurLumTextureSys = CTexture::Create2DTexture("$HDRCurrLuminanceSys", 1, 1, 0, dwFlagsDyn, 0, /*r->m_HDR_FloatFormat_Scalar*/eTF_G16R16F, eTF_G16R16F);//r->m_HDR_FloatFormat_Scalar);
	
  // For each scale stage, create a texture to hold the intermediate results
  // of the luminance calculation
  for(i=0; i<NUM_HDR_TONEMAP_TEXTURES; i++)
  {
    int iSampleLen = 1 << (2*i);
    sprintf(szName, "$HDRToneMap_%d", i);
    AllocateDeferredRenderTarget(iSampleLen, iSampleLen, eTF_G16R16F/*r->m_HDR_FloatFormat_Scalar*/, 0.7f, szName, &s_ptexHDRToneMaps[i], 0, -1, true);
  }

  EndRenderTargetList();

	r->m_vSceneLuminanceInfo = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	r->m_fAdaptedSceneScaleLBuffer = r->m_fAdaptedSceneScale = r->m_fStocopicSceneScale = 1.0f;

  // Create resources if necessary - todo: refactor all this shared render targets stuff, quite cumbersome atm...
  SPostEffectsUtils::Create(); 
}

void DrawQuad3D(float s0, float t0, float s1, float t1)
{
  const float fZ=0.5f;
  const float fX0=-1.0f, fX1=1.0f;
#if defined(OPENGL)
  const float fY0=-1.0f, fY1=1.0f;
#else
  const float fY0=1.0f, fY1=-1.0f;
#endif
  gcpRendD3D->DrawQuad3D(Vec3(fX0,fY1,fZ), Vec3(fX1,fY1,fZ), Vec3(fX1,fY0,fZ), Vec3(fX0,fY0,fZ), Col_White, s0, t0, s1, t1);
}

void DrawFullScreenQuadTR(float xpos, float ypos, float w, float h)
{
  int nOffs;
  SVF_P3F_C4B_T2F *vQuad = (SVF_P3F_C4B_T2F *)gcpRendD3D->GetVBPtr(4, nOffs);

  DWORD col = ~0;

  // Now that we have write access to the buffer, we can copy our vertices
  // into it

  const float s0 = 0;
  const float s1 = 1;
#if defined(OPENGL)
  const float t0 = 0;
  const float t1 = 1;
#else
  const float t0 = 1;
  const float t1 = 0;
#endif

  // Define the quad
  vQuad[0].xyz = Vec3(xpos, ypos, 1.0f);
  vQuad[0].color.dcolor = col;
  vQuad[0].st = Vec2(s0, 1.0f-t0);

  vQuad[1].xyz = Vec3(xpos+w, ypos, 1.0f);
  vQuad[1].color.dcolor = col;
  vQuad[1].st = Vec2(s1, 1.0f-t0);

  vQuad[3].xyz = Vec3(xpos+w, ypos+h, 1.0f);
  vQuad[3].color.dcolor = col;
  vQuad[3].st = Vec2(s1, 1.0f-t1);

  vQuad[2].xyz = Vec3(xpos, ypos+h, 1.0f);
  vQuad[2].color.dcolor = col;
  vQuad[2].st = Vec2(s0, 1.0f-t1);

  // We are finished with accessing the vertex buffer
  gcpRendD3D->UnlockVB();

  gcpRendD3D->FX_Commit();

  // Bind our vertex as the first data stream of our device
  gcpRendD3D->FX_SetVStream(0, gcpRendD3D->m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
  {
    // Render the two triangles from the data stream
  #if defined (DIRECT3D9) || defined (OPENGL)
    HRESULT hReturn = gcpRendD3D->m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
  #elif defined (DIRECT3D10)
    gcpRendD3D->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    gcpRendD3D->m_pd3dDeviceContext->Draw(4, nOffs);
  #endif
  }
}

#define NV_CACHE_OPTS_ENABLED TRUE

//-----------------------------------------------------------------------------
// Name: DrawFullScreenQuad
// Desc: Draw a properly aligned quad covering the entire render target
//-----------------------------------------------------------------------------
void DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV, bool bClampToScreenRes)
{
//#ifndef XENON
  D3DSURFACE_DESC dtdsdRT;
  memset(&dtdsdRT, 0, sizeof(dtdsdRT));
  PDIRECT3DSURFACE9 pSurfRT = NULL;
  CD3D9Renderer *rd = gcpRendD3D;

  rd->FX_Commit();

  // Acquire render target width and height
#if defined (DIRECT3D9) || defined (OPENGL)
  rd->m_pd3dDevice->GetRenderTarget(0, &pSurfRT);
  pSurfRT->GetDesc(&dtdsdRT);
  pSurfRT->Release();
#elif defined (DIRECT3D10)
  dtdsdRT.Width = rd->m_NewViewport.nWidth;
  dtdsdRT.Height = rd->m_NewViewport.nHeight;
#endif

  bool bNV = NV_CACHE_OPTS_ENABLED;
#if defined (XENON) || defined (DIRECT3D10)
  bNV = false;
#endif
  // Ensure that we're directly mapping texels to pixels by offset by 0.5
  // For more info see the doc page titled "Directly Mapping Texels to Pixels"
  int nWidth = dtdsdRT.Width;
  int nHeight = dtdsdRT.Height;
	if(bClampToScreenRes)
	{
		nWidth = min(nWidth, rd->GetWidth());
		nHeight = min(nHeight, rd->GetHeight());
	}

  float fWidth5 = (float)nWidth;
  float fHeight5 = (float)nHeight;
  fWidth5 = (bNV) ?  (2.f*fWidth5) - 0.5f : fWidth5 - 0.5f;
  fHeight5 = (bNV) ? (2.f*fHeight5)- 0.5f : fHeight5 - 0.5f;

  // Draw the quad
  int nOffs;
  SVF_TP3F_C4B_T2F *Verts = (SVF_TP3F_C4B_T2F *)rd->GetVBPtr(bNV?3:4, nOffs, POOL_TRP3F_COL4UB_TEX2F);

#if defined (DIRECT3D10) || defined (XENON)
  fTopV = 1 - fTopV;
  fBottomV = 1 - fBottomV;
#endif







  Verts[0].pos = Vec4(-0.5f, -0.5f, 0.0f, 1.0f);
  Verts[0].color.dcolor = ~0;
  Verts[0].st = Vec2(fLeftU, fTopV);
  if (bNV)
  {
    float tWidth = fRightU - fLeftU;
    float tHeight = fBottomV - fTopV;
    Verts[1].pos = Vec4(fWidth5, -0.5f, 0.0f, 1.f);
    Verts[1].color.dcolor = ~0;
    Verts[1].st = Vec2(fLeftU + (tWidth*2.f), fTopV);

    Verts[2].pos = Vec4(-0.5f, fHeight5, 0.0f, 1.f);
    Verts[2].color.dcolor = ~0;
    Verts[2].st = Vec2(fLeftU, fTopV + (tHeight*2.f));
  }
  else
  {
    Verts[1].pos = Vec4(fWidth5, -0.5f, 0.0f, 1.0f);
    Verts[1].color.dcolor = ~0;
    Verts[1].st = Vec2(fRightU, fTopV);

    Verts[2].pos = Vec4(-0.5f, fHeight5, 0.0f, 1.0f);
    Verts[2].color.dcolor = ~0;
    Verts[2].st = Vec2(fLeftU, fBottomV);

    Verts[3].pos = Vec4(fWidth5, fHeight5, 0.0f, 1.0f);
    Verts[3].color.dcolor = ~0;
    Verts[3].st = Vec2(fRightU, fBottomV);
  }




  rd->UnlockVB(POOL_TRP3F_COL4UB_TEX2F);

  rd->EF_SetState(GS_NODEPTHTEST);
  if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_TP3F_C4B_T2F)))
  {
    rd->FX_SetVStream(0, rd->m_pVB[POOL_TRP3F_COL4UB_TEX2F], 0, sizeof(SVF_TP3F_C4B_T2F));
  #if defined (DIRECT3D9) || defined (OPENGL)
    rd->m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, (bNV)?1:2);
  #elif defined (DIRECT3D10)
    rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    rd->m_pd3dDeviceContext->Draw((bNV)?3:4, nOffs);
  #endif

    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_GENERAL] += (bNV)?1:2;
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_GENERAL]++;
  }
}

void DrawFullScreenQuad(CoordRect c, bool bClampToScreenRes)
{
  DrawFullScreenQuad( c.fLeftU, c.fTopV, c.fRightU, c.fBottomV, bClampToScreenRes );
}


//-----------------------------------------------------------------------------
// Name: GetTextureRect()
// Desc: Get the dimensions of the texture
//-----------------------------------------------------------------------------
HRESULT GetTextureRect(CTexture *pTexture, RECT* pRect)
{
  pRect->left = 0;
  pRect->top = 0;
  pRect->right = pTexture->GetWidth();
  pRect->bottom = pTexture->GetHeight();

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: GetTextureCoords()
// Desc: Get the texture coordinates to use when rendering into the destination
//       texture, given the source and destination rectangles
//-----------------------------------------------------------------------------
HRESULT GetTextureCoords(CTexture *pTexSrc, RECT* pRectSrc, CTexture *pTexDest, RECT* pRectDest, CoordRect* pCoords)
{
  float tU, tV;

  // Validate arguments
  if( pTexSrc == NULL || pTexDest == NULL || pCoords == NULL )
    return E_INVALIDARG;

  // Start with a default mapping of the complete source surface to complete 
  // destination surface
  pCoords->fLeftU = 0.0f;
  pCoords->fTopV = 0.0f;
  pCoords->fRightU = 1.0f; 
  pCoords->fBottomV = 1.0f;

  // If not using the complete source surface, adjust the coordinates
  if(pRectSrc != NULL)
  {
    // These delta values are the distance between source texel centers in 
    // texture address space
    tU = 1.0f / pTexSrc->GetWidth();
    tV = 1.0f / pTexSrc->GetHeight();

    pCoords->fLeftU += pRectSrc->left * tU;
    pCoords->fTopV += pRectSrc->top * tV;
    pCoords->fRightU -= (pTexSrc->GetWidth() - pRectSrc->right) * tU;
    pCoords->fBottomV -= (pTexSrc->GetHeight() - pRectSrc->bottom) * tV;
  }

  // If not drawing to the complete destination surface, adjust the coordinates
  if(pRectDest != NULL)
  {
    // These delta values are the distance between source texel centers in 
    // texture address space
    tU = 1.0f / pTexDest->GetWidth();
    tV = 1.0f / pTexDest->GetHeight();

    pCoords->fLeftU -= pRectDest->left * tU;
    pCoords->fTopV -= pRectDest->top * tV;
    pCoords->fRightU += (pTexDest->GetWidth() - pRectDest->right) * tU;
    pCoords->fBottomV += (pTexDest->GetHeight() - pRectDest->bottom) * tV;
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale4x4
// Desc: Get the texture coordinate offsets to be used inside the DownScale4x4
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT GetSampleOffsets_DownScale4x4( DWORD dwWidth, DWORD dwHeight, Vec4 avSampleOffsets[])
{
  if(NULL == avSampleOffsets)
    return E_INVALIDARG;

  float tU = 1.0f / dwWidth;
  float tV = 1.0f / dwHeight;

  // Sample from the 16 surrounding points. Since the center point will be in
  // the exact center of 16 texels, a 0.5f offset is needed to specify a texel
  // center.
  int index=0;
  for(int y=0; y<4; y++)
  {
    for(int x=0; x<4; x++)
    {
      avSampleOffsets[index].x = (x - 1.5f) * tU;
      avSampleOffsets[index].y = (y - 1.5f) * tV;
      avSampleOffsets[index].z = 0;
      avSampleOffsets[index].w = 1;

      index++;
    }
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale4x4Bilinear
// Desc: Get the texture coordinate offsets to be used inside the DownScale4x4
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT GetSampleOffsets_DownScale4x4Bilinear( DWORD dwWidth, DWORD dwHeight, Vec4 avSampleOffsets[])
{
  if ( NULL == avSampleOffsets )
    return E_INVALIDARG;

  float tU = 1.0f / dwWidth;
  float tV = 1.0f / dwHeight;

  // Sample from the 16 surrounding points.  Since bilinear filtering is being used, specific the coordinate
  // exactly halfway between the current texel center (k-1.5) and the neighboring texel center (k-0.5)

  int index=0;
  for(int y=0; y<4; y+=2)
  {
    for(int x=0; x<4; x+=2, index++)
    {
      avSampleOffsets[index].x = (x - 1.f) * tU;
      avSampleOffsets[index].y = (y - 1.f) * tV;
      avSampleOffsets[index].z = 0;
      avSampleOffsets[index].w = 1;
    }
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale2x2
// Desc: Get the texture coordinate offsets to be used inside the DownScale2x2
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT GetSampleOffsets_DownScale2x2( DWORD dwWidth, DWORD dwHeight, Vec4 avSampleOffsets[] )
{
  if( NULL == avSampleOffsets )
    return E_INVALIDARG;

  float tU = 1.0f / dwWidth;
  float tV = 1.0f / dwHeight;

  // Sample from the 4 surrounding points. Since the center point will be in
  // the exact center of 4 texels, a 0.5f offset is needed to specify a texel
  // center.
  int index=0;
  for( int y=0; y < 2; y++ )
  {
    for( int x=0; x < 2; x++ )
    {
      avSampleOffsets[index].x = (x - 0.5f) * tU;
      avSampleOffsets[index].y = (y - 0.5f) * tV;
      avSampleOffsets[index].z = 0;
      avSampleOffsets[index].w = 1;

      index++;
    }
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: GaussianDistribution
// Desc: Helper function for GetSampleOffsets function to compute the 
//       2 parameter Gaussian distribution using the given standard deviation
//       rho
//-----------------------------------------------------------------------------
float GaussianDistribution( float x, float y, float rho )
{
	float g = 1.0f / sqrtf(2.0f * D3DX_PI * rho * rho);
	g *= expf( -(x*x + y*y)/(2*rho*rho) );

	return g;
}

//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_GaussBlur5x5
// Desc: Get the texture coordinate offsets to be used inside the GaussBlur5x5
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT GetSampleOffsets_GaussBlur5x5(DWORD dwD3DTexWidth, DWORD dwD3DTexHeight, Vec4* avTexCoordOffset, Vec4* avSampleWeight, FLOAT fMultiplier)
{
	float tu = 1.0f / (float)dwD3DTexWidth ;
	float tv = 1.0f / (float)dwD3DTexHeight ;

	Vec4 vWhite( 1.0f, 1.0f, 1.0f, 1.0f );

	float totalWeight = 0.0f;
	int index=0;
	for(int x=-2; x<=2; x++)
	{
		for(int y=-2; y<=2; y++)
		{
			// Exclude pixels with a block distance greater than 2. This will
			// create a kernel which approximates a 5x5 kernel using only 13
			// sample points instead of 25; this is necessary since 2.0 shaders
			// only support 16 texture grabs.
			if( abs(x) + abs(y) > 2 )
				continue;

			// Get the unscaled Gaussian intensity for this offset
			avTexCoordOffset[index] = Vec4(x*tu, y*tv, 0, 1);
			avSampleWeight[index] = vWhite * GaussianDistribution( (float)x, (float)y, 1.0f);
			totalWeight += avSampleWeight[index].x;

			index++;
		}
	}

	// Divide the current weight by the total weight of all the samples; Gaussian
	// blur kernels add to 1.0f to ensure that the intensity of the image isn't
	// changed when the blur occurs. An optional multiplier variable is used to
	// add or remove image intensity during the blur.
	for(int i=0; i<index; i++)
	{
		avSampleWeight[i] /= totalWeight;
		avSampleWeight[i] *= fMultiplier;
	}

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void HDR_SceneToSceneScaled()
{  
  PROFILE_LABEL_PUSH( "SCENETOSCENE_SCALED" );
	
	gcpRendD3D->Set2DMode(true, 1, 1);
	






	GetUtils().StretchRect( CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetScaled[0]);	


	gcpRendD3D->Set2DMode(false, 1, 1);

  PROFILE_LABEL_POP( "SCENETOSCENE_SCALED" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void HDR_MeasureLuminance(CShader *pSH)
{
  PROFILE_LABEL_PUSH( "MEASURE_LUMINANCE" );
  HRESULT hr = S_OK;
  int x, y, index;
  Vec4 avSampleOffsets[16];
  CD3D9Renderer *rd = gcpRendD3D;

	uint64 nFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
	gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]);
	





  int32 dwCurTexture = NUM_HDR_TONEMAP_TEXTURES-1;
  static CCryName Param1Name("SampleOffsets");

  // Initialize the sample offsets for the initial luminance pass.
  float tU, tV;
  tU = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth());
  tV = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());

  index=0;
  for(x=-1; x<=1; x++)
  {
    for(y=-1; y<=1; y++)
    {
      avSampleOffsets[index].x = x * tU;
      avSampleOffsets[index].y = y * tV;
      avSampleOffsets[index].z = 0;
      avSampleOffsets[index].w = 1;

      index++;
    }
  }

	ColorF clearColor(0, 0, 0, 0);
	uint32 nPasses;

	rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[dwCurTexture], NULL);
	rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());        
  
	static CCryName TechName("HDRSampleLumInitial");
	pSH->FXSetTechnique(TechName);  
	pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	pSH->FXBeginPass(0);

  CTexture::s_ptexHDRTargetScaled[0]->Apply(0, nTexStateLinear /*nTexStatePoint*/);

	float s1 = 1.0f / (float) CTexture::s_ptexHDRTargetScaled[0]->GetWidth();
	float t1 = 1.0f / (float) CTexture::s_ptexHDRTargetScaled[0]->GetHeight();       

	// Use rotated grid
	Vec4 vSampleLumOffsets0 = Vec4(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
	Vec4 vSampleLumOffsets1 = Vec4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  

	static CCryName pSampleLumOffsetsName0("SampleLumOffsets0");
	static CCryName pSampleLumOffsetsName1("SampleLumOffsets1");

	pSH->FXSetPSFloat(pSampleLumOffsetsName0, &vSampleLumOffsets0, 1);        
	pSH->FXSetPSFloat(pSampleLumOffsetsName1, &vSampleLumOffsets1, 1); 

	SetHDRParams(pSH);

  // Draw a fullscreen quad to sample the RT
  DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

  pSH->FXEndPass();

	rd->FX_PopRenderTarget(0);

  dwCurTexture--;

  // Initialize the sample offsets for the iterative luminance passes
  while(dwCurTexture >= 0)
  {
    rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[dwCurTexture], NULL);
		rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());        

		if( !dwCurTexture )
			gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    static CCryName TechNameLI("HDRSampleLumIterative");
    pSH->FXSetTechnique(TechNameLI);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);
    if (rd->m_bDeviceSupportsFP16Filter)
    {
      GetSampleOffsets_DownScale4x4Bilinear(CTexture::s_ptexHDRToneMaps[dwCurTexture+1]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture+1]->GetHeight(), avSampleOffsets);
      pSH->FXSetPSFloat(Param1Name, avSampleOffsets, 4);
      CTexture::s_ptexHDRToneMaps[dwCurTexture+1]->Apply(0, nTexStateLinear);
    }
    else
    {
      GetSampleOffsets_DownScale4x4(CTexture::s_ptexHDRToneMaps[dwCurTexture+1]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture+1]->GetHeight(), avSampleOffsets);

      pSH->FXSetPSFloat(Param1Name, avSampleOffsets, 16);
      CTexture::s_ptexHDRToneMaps[dwCurTexture+1]->Apply(0, nTexStatePoint);
    }

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

    pSH->FXEndPass();

		rd->FX_PopRenderTarget(0);

    dwCurTexture--;
  }

  PROFILE_LABEL_POP( "MEASURE_LUMINANCE" );





	gRenDev->m_RP.m_FlagsShader_RT = nFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void HDR_CalculateEyeAdaptation(CShader *pSH)
{
  PROFILE_LABEL_PUSH( "CALCULATE_EYEADAPTATION" );

  HRESULT hr = S_OK;
  CD3D9Renderer *rd = gcpRendD3D;

  // Swap current & last luminance
  int nCurLum = CTexture::s_nCurLumTextureIndex++;
  nCurLum &= 7;
  CTexture *pTexPrev = CTexture::s_ptexHDRAdaptedLuminanceCur[(nCurLum-1)&7];
  CTexture *pTexCur = CTexture::s_ptexHDRAdaptedLuminanceCur[nCurLum];
  CTexture::s_ptexCurLumTexture = pTexCur;
  assert(pTexCur);

  uint32 nPasses;
  static CCryName TechName("HDRCalculateAdaptedLum");
  pSH->FXSetTechnique(TechName);
  pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

  rd->FX_PushRenderTarget(0, pTexCur, NULL);
	rd->RT_SetViewport(0, 0, pTexCur->GetWidth(), pTexCur->GetHeight());        

  pSH->FXBeginPass(0);

  Vec4 v;
  v[0] = iTimer->GetFrameTime();
	v[1] = 1 - powf( 0.98f, CRenderer::CV_r_HDREyeAdaptationSpeed * 33.0f * v[0]);
  v[2] = 1 - powf( 0.98f, CRenderer::CV_r_HDRRangeAdaptationSpeed * 33.0f * v[0]);
  v[3] = 0;
  static CCryName Param1Name("ElapsedTime");
  pSH->FXSetPSFloat(Param1Name, &v, 1);
  pTexPrev->Apply(0, nTexStatePoint);
  CTexture::s_ptexHDRToneMaps[0]->Apply(1, nTexStatePoint);

  // Draw a fullscreen quad to sample the RT
  DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

  pSH->FXEndPass();

	rd->FX_PopRenderTarget(0);

  PROFILE_LABEL_POP( "CALCULATE_EYEADAPTATION" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_HDRRangeAdaptUpdate()
{
	HRESULT hr = S_OK;
	CD3D9Renderer *rd = gcpRendD3D;

	if( CRenderer:: CV_r_HDRRangeAdapt ) 
	{
		// Swap current & last luminance
		int nCurLum = CTexture::s_nCurLumTextureIndex;
		CTexture *pTexPrev = CTexture::s_ptexHDRAdaptedLuminanceCur[(nCurLum-2)&7];

		STALL_PROFILER("update scene luminance")

		CTexture *pTex = pTexPrev;//CTexture::s_ptexCurLumTexture;
		CDeviceTexture *pSrcDevTex = pTex->GetDevTexture();//CTexture::s_ptexCurLumTexture->GetDevTexture();
		CDeviceTexture *pDstDevTex = CTexture::s_ptexCurLumTextureSys->GetDevTexture();		  
		assert(pSrcDevTex);
		assert(pDstDevTex);

		//D3DSurface *pSrcDevSurf = pTex->GetSurface(0, 0);
		//D3DSurface *pDstDevSurfSys = CTexture::s_ptexCurLumTextureSys->GetSurface(0, 0);

		D3DXFLOAT16 *hSurfSys = 0;
		float pLumInfo[2];
		pLumInfo[0] = pLumInfo[1] = 0;
		CryHalf *pRawPtr = 0;







#if defined(DIRECT3D9)

		STexLock rect;  
		
#if !defined (XENON)
		//hr = rd->m_pd3dDevice->GetRenderTargetData(pSrcDevSurf, pDstDevSurfSys);
#endif
		if (SUCCEEDED(pSrcDevTex->LockRect(0, rect, LF_READ)) ) 
		{
			//hSurfSys = ((D3DXFLOAT16*)rect.pData);
			//D3DXFloat16To32Array (pLumInfo, hSurfSys, 2);

			pRawPtr = (CryHalf*) rect.pData;
			pLumInfo[0] = CryConvertHalfToFloat( pRawPtr[0] );
			pLumInfo[1] = CryConvertHalfToFloat( pRawPtr[1] );						

			pSrcDevTex->UnlockRect(0);

			//SAFE_RELEASE(pSrcDevSurf);
			//SAFE_RELEASE(pDstDevSurfSys);
		}

#endif
		
		// note: RG16F - channels are swapped 
		rd->m_vSceneLuminanceInfo.x = pLumInfo[1];	// avg
		rd->m_vSceneLuminanceInfo.y = pLumInfo[0];//0;//pLumInfo[2];	// min - instead of min, maybe we can simplify and output maybe 4x smaller luminance
		//rd->m_vSceneLuminanceInfo.z = ;	// max

		// For night/very dark scenes boost maximum range
		const float fStocopicThreshold = 0.01f;
		rd->m_fStocopicSceneScale += (( (rd->m_vSceneLuminanceInfo.x < fStocopicThreshold)? 2.0f : 1.0f) - rd->m_fStocopicSceneScale ) * gEnv->pTimer->GetFrameTime(); 

		// Tweakable scene range scale. 
		// Optimaly we could rescale to take advantage of full texture format range into account, but we need to maintain acceptable range for blooming (else we get very poor bloom quality)
		rd->m_fAdaptedSceneScale = CRenderer::CV_r_HDRRangeAdaptMax / (1e-6f + rd->m_vSceneLuminanceInfo.y);
		rd->m_fAdaptedSceneScale = max(1.0f, min(rd->m_fAdaptedSceneScale, CRenderer::CV_r_HDRRangeAdaptMaxRange * rd->m_fStocopicSceneScale) );

		// todo: we need to try out different strategies for light buffers (different scales for day/night scenes? we hit top clamping too often)
		
		// Light buffers range scale
		rd->m_fAdaptedSceneScaleLBuffer = CRenderer::CV_r_HDRRangeAdaptLBufferMax / (1e-6f + rd->m_vSceneLuminanceInfo.y);
		rd->m_fAdaptedSceneScaleLBuffer = max(1.0f, min(rd->m_fAdaptedSceneScaleLBuffer, CRenderer::CV_r_HDRRangeAdaptLBufferMaxRange * rd->m_fStocopicSceneScale) );
	}
	else
	{
		rd->m_vSceneLuminanceInfo = Vec4(1.0f, 1.0f, 1.0f, 1.0f);;
		rd->m_fAdaptedSceneScale = 1.0f;
		rd->m_fAdaptedSceneScaleLBuffer = 1.0f;
		rd->m_fStocopicSceneScale = 1.0f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void HDR_BrightPass(CShader *pSH)
{


  PROFILE_LABEL_PUSH( "BRIGHTPASS" );

	gcpRendD3D->Set2DMode(true, 1, 1);   

  HRESULT hr = S_OK;
  CD3D9Renderer *rd = gcpRendD3D;
  uint32 nPasses;

  CTexture *pShaftsRT = CTexture::s_ptexBlack;

  if( rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_LIGHTSHAFTS )
  {    
    //pShaftsRT = CTexture::s_ptexHDRTargetScaled[2];
    //GetUtils().TexBlurGaussian( pShaftsRT, 1, 1.0, 1.0f, false);
  }

  // When LDR glow enabled merge it with bloom
  CEffectParam *pParamGlowActive = PostEffectMgr().GetByName("Glow_Active"); 
  bool bGlowLDR = gRenDev->CV_r_PostProcess && CRenderer::CV_r_glow && ( pParamGlowActive->GetParam()>0.0f );

  uint64 nFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]);
	if(gRenDev->IsLinearSpaceShadingEnabled())
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0]; 

  if( bGlowLDR )
  {
    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    pParamGlowActive->SetParam(0);
  }

	// Avoid doing useless texture fetches..
	if( rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_LIGHTSHAFTS )
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];

	rd->FX_PushRenderTarget(0, CTexture::s_ptexBackBufferScaled[0], NULL);
	rd->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());        

  static CCryName TechName("HDRBrightPassFilter");
  pSH->FXSetTechnique(TechName);
  pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

  pSH->FXBeginPass(0);
  SetHDRParams(pSH);

  CTexture::s_ptexHDRTargetScaled[0]->Apply(0, nTexStatePoint);
  if (CTexture::s_ptexCurLumTexture && !CRenderer::CV_r_HDRRangeAdapt)
    CTexture::s_ptexCurLumTexture->Apply(1, nTexStatePoint);

  pShaftsRT->Apply(2, nTexStateLinear); 

	SPostEffectsUtils::DrawFullScreenQuad( CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight() );

  pSH->FXEndPass();

	rd->FX_PopRenderTarget(0);  
	gcpRendD3D->Set2DMode(false, 1, 1);

	gRenDev->m_RP.m_FlagsShader_RT = nFlagsShader_RT;

  PROFILE_LABEL_POP( "BRIGHTPASS" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void HDR_ProcessBloomAndFlares()
{
  PROFILE_LABEL_PUSH( "BLOOM_AND_FLARES" );

  CTexture::BindNULLFrom();

	uint32 nPasses = 0;
  gcpRendD3D->Set2DMode(true, 1, 1);      

  CD3D9Renderer *rd = gcpRendD3D; 

  // Bloom generation/intermediate render-targets
  CTexture *pBloomGen[3];         
  // Final bloom RT
  CTexture *pBloom = 0; 

	// make cvar - like bloom quality (0, 1)

	pBloomGen[0] = CTexture::s_ptexBackBufferScaled[0];//CTexture::s_ptexHDRBrightPass[0];           // 4x smaller than frame buffer - CTexture::s_ptexHDRTargetScaled[1];  // 4x
	pBloomGen[1] = CTexture::s_ptexBackBufferScaled[1];  // 8x
	pBloomGen[2] = CTexture::s_ptexBackBufferScaled[2];  // 16x

  // Bloom/Glow generation: 
  //  - using 3 textures, each with half resolution of previous, Gaussian blurred and result summed up at end

	if( CRenderer::CV_r_glow >= 2)
		GetUtils().TexBlurIterative(pBloomGen[0], CRenderer::CV_r_glow - 1);
	else
		GetUtils().TexBlurGaussian( pBloomGen[0], 1, 1.0f, 2.0f, false, 0, true);                  // rad = 2
	






  GetUtils().StretchRect( pBloomGen[0], pBloomGen[1] );      


	// multiply second bloom step by 2.0 - trying to maintain a bit more precision on darks
	// - alternatives: use HW SRGB (in shader encoding is too expensive) or FP format
	GetUtils().TexBlurGaussian( pBloomGen[1], 1, 2.0f, 5.0f, false, 0, true);            // rad = 5







  GetUtils().StretchRect( pBloomGen[1], pBloomGen[2] );                 


  GetUtils().TexBlurGaussian( pBloomGen[2], 1, 1.0f, 5.0f, false, 0, true);     //rad = 5

  gcpRendD3D->Set2DMode(false, 1, 1);      

  PROFILE_LABEL_POP( "BLOOM_AND_FLARES" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void HDR_RenderFinalScene(CShader *pSH)
{
  PROFILE_LABEL_PUSH( "SCENE_TONEMAP" );

	HRESULT hr = S_OK;  
	CD3D9Renderer *rd = gcpRendD3D;

	if (!gcpRendD3D->m_pColorGradingControllerD3D)
		return;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Update color grading - keep code - when we have 3D lookup we can try out again HDR merge for profiling

	//CColorGrading *pColorGrad = 0;
	//if( !PostEffectMgr().GetEffects().empty() )
	//	pColorGrad =  static_cast<CColorGrading*>( PostEffectMgr().GetEffect(ePFX_ColorGrading) );

	// Enable corresponding shader variation
	gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]|g_HWSR_MaskBit[HWSR_SAMPLE4]);

	//SColorGradingMergeParams pMergeParams;
	//pColorGrad->UpdateParams( pMergeParams );

//	gRenDev->m_RP.m_FlagsShader_RT |= pMergeParams.nFlagsShaderRT & ( ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]) );
	if(gRenDev->IsLinearSpaceShadingEnabled())
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0]; 
	if(CRenderer::CV_r_HDRBlueShift)
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1]; 
	if(CRenderer::CV_r_HDRRangeAdapt)
		gRenDev->m_RP.m_FlagsShader_RT |=	g_HWSR_MaskBit[HWSR_SAMPLE4];

	//CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D; 
	//CTexture *pTexColorChar = pCtrl? pCtrl->GetColorChart() : 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Final bloom RT

	CTexture *pBloom[3];
	if( CRenderer::CV_r_glow )
	{
		pBloom[0] = CTexture::s_ptexBackBufferScaled[0];
		pBloom[1] = CTexture::s_ptexBackBufferScaled[1];
		pBloom[2] = CTexture::s_ptexBackBufferScaled[2];
	}
	else
		pBloom[0] = pBloom[1] = pBloom[2] = CTexture::s_ptexBlack;

  uint32 nPasses;
  static CCryName TechFinalName("HDRFinalPass");
  pSH->FXSetTechnique(TechFinalName);
  pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  pSH->FXBeginPass(0);

	SetHDRParams(pSH);

	Vec4 vHDRParams3 = Vec4(1.0f - clamp_tpl<float>(CRenderer::CV_r_HDRBlueShift, 0.0f, 1.0f), 1, 0, 0);
	CCryName pszHDRParam3("HDRParams3");
	pSH->FXSetPSFloat(pszHDRParam3, &vHDRParams3, 1);

  CTexture::s_ptexHDRTarget->Apply(0, nTexStatePoint);
	if (CTexture::s_ptexCurLumTexture && !CRenderer::CV_r_HDRRangeAdapt)
    CTexture::s_ptexCurLumTexture->Apply(1, nTexStatePoint);

	pBloom[0]->Apply(2, nTexStateLinear);
	pBloom[1]->Apply(3, nTexStateLinear);
	pBloom[2]->Apply(4, nTexStateLinear);

	CTexture::s_ptexBlack->Apply(5, nTexStatePoint);

	//if( pTexColorChar )
		//pTexColorChar->Apply(5, nTexStateLinear);

  static uint32 dwNoiseOffsetX=0;
  static uint32 dwNoiseOffsetY=0;

  dwNoiseOffsetX = (dwNoiseOffsetX+27)&0x3f;
  dwNoiseOffsetY = (dwNoiseOffsetY+19)&0x3f;

  Vec4 pFrameRand(dwNoiseOffsetX/64.0f, dwNoiseOffsetX/64.0f, (cry_rand()%1024)/1024.0f, (cry_rand()%1024)/1024.0f );
  static CCryName Param4Name("FrameRand");
  pSH->FXSetVSFloat(Param4Name, &pFrameRand, 1);

	CTexture::s_ptexScreenNoiseMap->Apply(6, nTexStatePointWrap);

	if( gRenDev->CV_r_PostProcess && CRenderer::CV_r_HDRVignetting )
		CTexture::s_ptexVignettingMap->Apply(7, nTexStateLinear);
	else
		CTexture::s_ptexWhite->Apply(7, nTexStateLinear);

	DrawQuad3D(0, 1, 1, 0);

  rd->EF_SelectTMU(0);

  PROFILE_LABEL_POP( "SCENE_TONEMAP" );
}

void HDR_RenderFinalDebugScene(CShader *pSH)
{
  HRESULT hr = S_OK;
  CD3D9Renderer *rd = gcpRendD3D;
  Vec4 avSampleOffsets[4];

  uint32 nPasses;
	static CCryName techName("HDRFinalDebugPass");
  pSH->FXSetTechnique(techName);
  pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  pSH->FXBeginPass(0);

  GetSampleOffsets_DownScale2x2(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight(), avSampleOffsets);
 	static CCryName SampleOffsetsName("SampleOffsets");
  pSH->FXSetPSFloat(SampleOffsetsName, avSampleOffsets, 4);

  if (rd->m_nHDRType > 1)
    CTexture::s_ptexHDRTarget->Apply(0, nTexStateLinear);
  else
    CTexture::s_ptexHDRTarget->Apply(0, nTexStatePoint);

  DrawFullScreenQuad( 0.0f, 0.0f, 1.0f, 1.0f );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void HDR_DrawDebug()
{
#ifdef USE_HDR
  if (!CRenderer::CV_r_HDRRendering)
    return;
#endif

  CD3D9Renderer *rd = gcpRendD3D;
	uint32 nPasses = 0;

  rd->EF_SetState(GS_NODEPTHTEST);
  int iTmpX, iTmpY, iTempWidth, iTempHeight;
  rd->GetViewport(&iTmpX, &iTmpY, &iTempWidth, &iTempHeight);   
	
  rd->EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
  rd->Set2DMode(true, 1, 1);

  CShader *pSH = CShaderMan::m_ShaderDebug;

  // 1
	uint32 nPosX = 10;
  rd->RT_SetViewport(nPosX, 400, 100, 100);   
  rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTarget->GetID(), 0, 1, 1, 0, 1,1,1,1);

  // 2
  rd->RT_SetViewport(nPosX += 110, 400, 100, 100);   
  rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTargetScaled[0]->GetID(), 0, 1, 1, 0, 1,1,1,1);

  // 3
  if( CRenderer::CV_r_glow )
  {    
    // Bloom generation/intermediate render-targets
    CTexture *pBloomGen[3];         
    // Final bloom RT
    CTexture *pBloom = 0; 
    
    pBloom = CTexture::s_ptexHDRTargetScaled[0];             // 2x smaller than frame buffer
    pBloomGen[0] = CTexture::s_ptexBackBufferScaled[0];  // 2x         ''
    pBloomGen[1] = CTexture::s_ptexBackBufferScaled[1];  // 4x         ''
    pBloomGen[2] = CTexture::s_ptexBackBufferScaled[2];  // 8x         ''
 
    rd->RT_SetViewport(nPosX += 110, 400, 100, 100);   
    rd->DrawImage(0, 0, 1, 1, pBloom->GetID(), 0, 1, 1, 0, 1,1,1,1);

    rd->RT_SetViewport(nPosX += 110, 400, 100, 100);
    rd->DrawImage(0, 0, 1, 1, pBloomGen[0]->GetID(), 0, 1, 1, 0, 1,1,1,1);

    rd->RT_SetViewport(nPosX += 110, 400, 100, 100);
    rd->DrawImage(0, 0, 1, 1, pBloomGen[1]->GetID(), 0, 1, 1, 0, 1,1,1,1);

    rd->RT_SetViewport(nPosX += 110, 400, 100, 100);
    rd->DrawImage(0, 0, 1, 1, pBloomGen[2]->GetID(), 0, 1, 1, 0, 1,1,1,1);
  }

	nPosX = 10;

	pSH->FXSetTechnique("Debug_ShowR");
	pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	pSH->FXBeginPass(0);

	CTexture::s_ptexHDRToneMaps[3]->Apply(0, nTexStatePoint);
	rd->RT_SetViewport(nPosX, 510, 100, 100);   
	DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);  

	if (CTexture::s_ptexHDRToneMaps[2])
	{
		CTexture::s_ptexHDRToneMaps[2]->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 510, 100, 100);   
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}

	if (CTexture::s_ptexHDRToneMaps[1])
	{
		CTexture::s_ptexHDRToneMaps[1]->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 510, 100, 100);   
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}

	if (CTexture::s_ptexHDRToneMaps[0])
	{
		CTexture::s_ptexHDRToneMaps[0]->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 510, 100, 100);   
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}

	if (CTexture::s_ptexCurLumTexture)
	{
		CTexture::s_ptexCurLumTexture->Apply(0, nTexStatePoint);
		rd->RT_SetViewport(nPosX += 110, 510, 100, 100);   
		DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
	}
	
	pSH->FXEndPass();
	pSH->FXEnd();

  rd->Set2DMode(false, 1, 1);
  rd->RT_SetViewport(iTmpX, iTmpY, iTempWidth, iTempHeight);   

	{
		char str[256];

		SDrawTextInfo ti;

		sprintf(str,"r_HDRRendering: %d", CRenderer::CV_r_HDRRendering);
		rd->Draw2dText(5,230,str,ti);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::FX_HDRPostProcessing()
{
  PROFILE_FRAME(Draw_HDR_PostProcessing);

  nTexStateLinear = CTexture::GetTexState(sTexStateLinear);
  nTexStateLinearWrap = CTexture::GetTexState(sTexStateLinearWrap);
  nTexStatePoint = CTexture::GetTexState(sTexStatePoint);
	nTexStatePointWrap = CTexture::GetTexState(sTexStatePointWrap);

  if (m_wireframe_mode == R_WIREFRAME_MODE)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#elif defined (DIRECT3D10)
		SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
		RS.Desc.FillMode = D3D11_FILL_SOLID;
		gcpRendD3D->SetRasterState(&RS);
#endif
  }

  if (!CTexture::IsTextureExist(CTexture::s_ptexHDRTarget))
   return;
	if (CTexture::s_ptexHDRTarget->GetWidth() != GetWidth() || CTexture::s_ptexHDRTarget->GetHeight() != GetHeight() || CRenderer::CV_r_HDRForceUpdateTextures)
	{
    CTexture::GenerateHDRMaps();
		CRenderer::CV_r_HDRForceUpdateTextures = 0;
	}

  PROFILE_LABEL_PUSH( "HDR_POSTPROCESS" );

  FX_ResetPipe();





#if !defined(XENON) && !defined(PS3)
  // No HDR post effects for 360 - for now
  FX_PostProcessSceneHDR();
#endif

  assert(m_RTStack[0][m_nRTStackLevel[0]].m_pTex == CTexture::s_ptexHDRTarget);

  CTexture::s_ptexHDRTarget->Resolve();

  if(CRenderer::CV_r_GetScreenShot==1)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
		char path[ICryPak::g_nMaxPath];
		path[sizeof(path) - 1] = 0;
		gEnv->pCryPak->AdjustFileName("%USER%/ScreenShots", path, ICryPak::FLAGS_PATH_REAL|ICryPak::FLAGS_FOR_WRITING);

		if (!gEnv->pCryPak->MakeDir(path))
		{
			iLog->Log("Cannot save screen shot! Failed to create directory \"%s\".", path);
		}
		else
		{
			char pszFileName[1024];

			int i=0;
			for(i=0 ; i<10000; i++)
			{
				snprintf(pszFileName, sizeof(pszFileName), "%s/ScreenShot%04d.hdr", path, i);
				pszFileName[sizeof(pszFileName)-1] = '\0';

				FILE *fp= fxopen(pszFileName, "rb");
				if(!fp)
					break;

				fclose(fp);
			}

			if(i==10000)
			{
				iLog->LogError("Cannot save ScreenShot: Too many HDR files");
				return;
			}


			D3DSurface *pHDRSurf = CTexture::s_ptexHDRTarget->GetSurface(-1, 0);
			D3DXSaveSurfaceToFile(pszFileName, D3DXIFF_HDR, pHDRSurf, 0, 0);
			SAFE_RELEASE(pHDRSurf);
		}
#else
		iLog->LogError("HDR screen shots are not yet supported on DX11!");		// TODO: D3DXSaveSurfaceToFile()
#endif
  }

  CShader *pSH = CShaderMan::m_shHDRPostProcess;
  m_RP.m_FlagsShader_RT = 0;

  if (gRenDev->IsLinearSpaceShadingEnabled())
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
	if(CRenderer::CV_r_HDRRangeAdapt)
		gRenDev->m_RP.m_FlagsShader_RT |=	g_HWSR_MaskBit[HWSR_SAMPLE4];

  FX_ApplyShaderQuality(eST_HDR);  

  // Create a scaled copy of the scene
  HDR_SceneToSceneScaled();

  // Setup tone mapping technique
  HDR_MeasureLuminance(pSH);

  HDR_CalculateEyeAdaptation(pSH);

  HDR_BrightPass(pSH);

  if( CRenderer::CV_r_glow )
    HDR_ProcessBloomAndFlares();
  
  FX_PopRenderTarget(0);
  EF_ClearBuffers(0, NULL);

  // Render final scene to the back buffer
  if (CRenderer::CV_r_HDRDebug != 1)
    HDR_RenderFinalScene(pSH);
  else
    HDR_RenderFinalDebugScene(pSH);

	if (CRenderer::CV_r_HDRDebug == 2 || CRenderer::CV_r_HDRDebug >= 4 )
		HDR_DrawDebug();	

	m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_HDR;
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~(RBPF2_HDR_FP16 | RBPF2_LIGHTSHAFTS);

  FX_ResetPipe();
  CTexture::BindNULLFrom();

	if(m_wireframe_mode == R_WIREFRAME_MODE)
	{
#if defined (DIRECT3D9) || defined (OPENGL)
    m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#elif defined (DIRECT3D10)
		SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
		RS.Desc.FillMode = D3D11_FILL_WIREFRAME;
		gcpRendD3D->SetRasterState(&RS);
#endif
  }
	else
  if(m_wireframe_mode == R_POINT_MODE)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);




#endif
  }

	if(CRenderer::CV_r_HDRDebug == 3 || CRenderer::CV_r_HDRDebug >= 4 )
	{
		char str[256];
		SDrawTextInfo ti;
		sprintf(str,"Avg Luminance: %.5f \nMin Luminance: %.5f \nAdapted scene scale: %.5f\nAdapted light buffer scale: %.5f",  
			m_vSceneLuminanceInfo.x, m_vSceneLuminanceInfo.y, m_fAdaptedSceneScale, m_fAdaptedSceneScaleLBuffer);
		Draw2dText(5,5,str,ti);
	}
	





  PROFILE_LABEL_POP( "HDR_POSTPROCESS" );
}
