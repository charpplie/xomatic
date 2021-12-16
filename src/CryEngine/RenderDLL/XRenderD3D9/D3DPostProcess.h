/*=============================================================================
D3DPostProcess : Direct3D specific post processing special effects
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

=============================================================================*/

#ifndef _D3DPOSTPROCESS_H_
#define _D3DPOSTPROCESS_H_

#include "../Common/PostProcess/PostEffects.h"

namespace NPostEffects
{
  
struct SD3DPostEffectsUtils: public SPostEffectsUtils
{
//  friend class CD3D9Renderer;

public:
  
  void CopyScreenToTexture(CTexture *&pDst);
  void StretchRect(CTexture *pSrc, CTexture *&pDst);
  void TexBlurIterative(CTexture *pTex, int nIterationsMul = 1); // 2 terations minimum (src => temp => src)
	void TexBlurGaussian(CTexture *pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly, CTexture *pMask = 0, bool bSRGB = false);
	void BilateralBlurring(CTexture *pSrc, CTexture *pDest);

	// Begins drawing a stencil pre-pass mask
	void BeginStencilPrePass( bool bAddToStencil = false, bool bDebug=false );
	// Ends drawing a stencil pre-pass mask
	void EndStencilPrePass( );
	// Setup render states for passes using stencil masks
	void SetupStencilStates( int32 nStFunc );













  SD3DSurface *GetDepthSurface( CTexture *pTex );
  void ResolveRT( CTexture *&pDst );

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  static SD3DPostEffectsUtils &GetInstance()
  {
    return m_pInstance;
  }


private:
  static SD3DPostEffectsUtils m_pInstance;

  SD3DPostEffectsUtils() { }
  virtual ~SD3DPostEffectsUtils() 
  { 
  }
};

#define GetUtils() SD3DPostEffectsUtils::GetInstance()

}

#endif