//////////////////////////////////////////////////////////////////////
//
//  Crytek CryENGINE Source code
//  
//  File: NULL_Shadows.cpp
//  Description: Implementation of the shadow maps using NULL device specific implementation
//  shadow map calculations
//
//  History:
//  -Jan 31,2001:Created by Vladimir Kajain
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "NULL_Renderer.h"
#include "../Common/Shadow_Renderer.h"

#include "I3DEngine.h"

IDynTexture *CNULLRenderer::MakeDynTextureFromShadowBuffer(int nSize, IDynTexture * pDynTexture)
{
  return NULL;
}

void CNULLRenderer::PrepareDepthMap(ShadowMapFrustum* SMSource, CDLight* pLight, int nFrustumLOD)
{
}

void CNULLRenderer::SetupShadowOnlyPass(int Num, ShadowMapFrustum* pShadowInst, Matrix34A * pObjMat)
{
}

void CNULLRenderer::DrawAllShadowsOnTheScreen()
{
}
