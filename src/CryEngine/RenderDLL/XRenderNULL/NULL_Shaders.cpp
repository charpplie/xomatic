/*=============================================================================
  NULL_Shaders.cpp : NULL device specific implementation effectors/shaders functions implementation.
  Copyright 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honitch Andrey

=============================================================================*/

#include "StdAfx.h"
#include "NULL_Renderer.h"
#include "I3DEngine.h"


#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

//============================================================================

bool CShader::FXSetTechnique(const CCryName& szName)
{
  return true;
}

bool CShader::FXSetPSFloat(const CCryName& NameParam, const Vec4* fParams, int nParams)
{
  return true;
}

bool CShader::FXSetVSFloat(const CCryName& NameParam, const Vec4* fParams, int nParams)
{
  return true;
}

bool CShader::FXBegin(uint32 *uiPassCount, uint32 nFlags)
{
  return true;
}

bool CShader::FXBeginPass(uint32 uiPass)
{
  return true;
}

bool CShader::FXEndPass()
{
  return true;
}

bool CShader::FXEnd()
{
  return true;
}

bool CShader::FXCommit(const uint32 nFlags)
{
  return true;
}

//===================================================================================

FXShaderCache CHWShader::m_ShaderCache;
FXShaderCacheNames CHWShader::m_ShaderCacheList;

SShaderCache::~SShaderCache()
{
  CHWShader::m_ShaderCache.erase(m_Name);
  SAFE_DELETE(m_pRes[CACHE_USER]);
  SAFE_DELETE(m_pRes[CACHE_READONLY]);
}

SShaderCache *CHWShader::mfInitCache(const char *name, CHWShader *pSH, bool bCheckValid, uint32 CRC32, bool bDontUseUserFolder, bool bReadOnly)
{
  return NULL;
}

bool CHWShader::mfOptimiseCacheFile(SShaderCache *pCache, bool bForce, SOptimiseStats *Stats)
{
  return true;
}

bool CHWShader::SetCurrentShaderCombinations(const char *szCombinations, bool bForLevel)
{
  bool bRes = true;
  return bRes;
}

const char *CHWShader::GetCurrentShaderCombinations(bool bLevel)
{
  return "";
}

void CHWShader::mfFlushPendedShadersWait(int nMaxAllowed)
{
}

void CHWShader::mfLazyUnload()
{
}

void CHWShader::mfBeginFrame(int nMaxToFlush)
{
}

void SRenderShaderResources::UpdateConstants(IShader *pSH)
{
}

void SRenderShaderResources::CloneConstants(const IRenderShaderResources* pSrc)
{
}

void SRenderShaderResources::RT_UpdateConstants(IShader *pISH)
{
}

void SRenderShaderResources::ReleaseConstants()
{
}

void CShader::mfFlushPendedShaders()
{
}
