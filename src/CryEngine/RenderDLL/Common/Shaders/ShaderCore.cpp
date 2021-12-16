/*=============================================================================
  ShaderCore.cpp : implementation of the Shaders manager.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "I3DEngine.h"
#include "CryHeaders.h"


CShader *CShaderMan::m_DefaultShader;
CShader *CShaderMan::m_shPostEffects;
CShader *CShaderMan::m_shPostDepthOfField;
CShader *CShaderMan::m_shPostMotionBlur; 
CShader *CShaderMan::m_shPostSunShafts; 
CShader *CShaderMan::m_sh3DHUD;
CShader *CShaderMan::m_shDeferredShading;
CShader *CShaderMan::m_ShaderDeferredCaustics;
CShader *CShaderMan::m_ShaderDeferredRain;
#ifndef NULL_RENDERER
CShader *CShaderMan::m_ShaderFPEmu;
CShader *CShaderMan::m_ShaderFallback;

CShader *CShaderMan::m_ShaderTreeSprites;
CShader *CShaderMan::m_ShaderShadowBlur;
CShader *CShaderMan::m_ShaderShadowMaskGen;
CShader *CShaderMan::m_ShaderAmbientOcclusion;
CShader *CShaderMan::m_ShaderScreenSpaceGI;
CShader *CShaderMan::m_ShaderCloudShadowMaskGen;
CShader *CShaderMan::m_shHDRPostProcess;
CShader *CShaderMan::m_ShaderDebug;
CShader *CShaderMan::m_ShaderLightFlares;
SShaderItem CShaderMan::m_ShaderLightStyles;
CShader *CShaderMan::m_shPostEffectsGame;
CShader *CShaderMan::m_ShaderCommon;
CShader *CShaderMan::m_ShaderOcclTest;
CShader *CShaderMan::m_ShaderDXTCompress = NULL;
CShader *CShaderMan::m_ShaderStereo = NULL;
#else
SShaderItem CShaderMan::m_DefaultShaderItem;
#endif
  
CCryNameCRC CShaderMan::s_cNameHEAD;

TArray<SRenderShaderResources *> CShader::m_ShaderResources_known;
TArray <CLightStyle *> CLightStyle::m_LStyles;

SResourceContainer *CShaderMan::m_pContainer;  // List/Map of objects for shaders resource class
FXCompressedShaders CHWShader::m_CompressedShaders;

uint64 g_HWSR_MaskBit[HWSR_MAX];

bool gbRgb;

//////////////////////////////////////////////////////////////////////////
// Global shader parser helper pointer.
CShaderParserHelper* g_pShaderParserHelper = 0;

//=================================================================================================

int CShader::GetTexId()
{
  CTexture *tp = (CTexture *)GetBaseTexture(NULL, NULL);
  if (!tp)
    return -1;
  return tp->GetTextureID();
}

#ifdef WIN64
#pragma warning( push )                 //AMD Port
#pragma warning( disable : 4267 )
#endif

int CShader::mfSize()
{
  uint32 i;

  int nSize = sizeof(CShader);
  nSize += m_NameFile.capacity();
  nSize += m_NameShader.capacity();
  nSize += m_HWTechniques.GetMemoryUsage();
  for (i=0; i<m_HWTechniques.Num(); i++)
  {
    nSize += m_HWTechniques[i]->Size();
  }

  nSize += m_PublicParams.size()*sizeof(SShaderParam);

  return nSize;
}

void CShader::GetMemoryUsage(ICrySizer* pSizer)
{
	pSizer->Add( *this );
	pSizer->AddObject( m_NameFile );
	pSizer->AddObject( m_NameShader );
	pSizer->AddObject( m_HWTechniques );			
	pSizer->AddObject( m_PublicParams );	
}

#ifdef WIN64
#pragma warning( pop )                  //AMD Port
#endif

void CShader::mfFree()
{
  uint32 i;

  for (i=0; i<m_HWTechniques.Num(); i++)
  {
    SShaderTechnique *pTech = m_HWTechniques[i];
#if defined (DIRECT3D10) || defined(PS3)
    uint32 j;
    for (j=0; j<pTech->m_Passes.size(); j++)
    {
      SShaderPass& Pass = pTech->m_Passes[j];
      if (Pass.m_VShader)
        UnregisterHW(Pass.m_VShader);
    }
#endif
    SAFE_DELETE(pTech);
  }
  m_HWTechniques.Free();

  SAFE_DELETE(m_ShaderGenParams);
  m_PublicParams.clear();
  m_Flags &= ~EF_PARSE_MASK;
  m_Flags2 &= ~(EF2_NODRAW);
  m_nMDV = 0;
}

CShader::~CShader()
{
  if (m_pGenShader && m_pGenShader->m_DerivedShaders)
  {
    uint32 i;
    for (i=0; i<m_pGenShader->m_DerivedShaders->Num(); i++)
    {
      CShader *pSH = (*m_pGenShader->m_DerivedShaders)[i];
      if (pSH == this)
      {
        (*m_pGenShader->m_DerivedShaders)[i] = NULL;
        break;
      }
    }
    assert(i != m_pGenShader->m_DerivedShaders->Num());
  }
  mfFree();

  SAFE_RELEASE(m_pGenShader);
  SAFE_DELETE(m_DerivedShaders);
}

#ifdef WIN64
#pragma warning( push )                 //AMD Port
#pragma warning( disable : 4311 )           // I believe the int cast below is okay.
#endif

CShader& CShader::operator = (const CShader& src)
{
  uint32 i;

  mfFree();
  
  int Offs = (int)(INT_PTR)&(((CShader *)0)->m_eSHDType);
  byte *d = (byte *)this;
  byte *s = (byte *)&src;
  memcpy(&d[Offs], &s[Offs], sizeof(CShader)-Offs);

  m_NameShader = src.m_NameShader;
  m_NameFile = src.m_NameFile;

  if (src.m_HWTechniques.Num())
  {
    m_HWTechniques.Create(src.m_HWTechniques.Num());
    for (i=0; i<src.m_HWTechniques.Num(); i++)
    {
      m_HWTechniques[i] = new SShaderTechnique;
      *m_HWTechniques[i] = *src.m_HWTechniques[i];
    }
  }

  m_PublicParams = src.m_PublicParams;

  return *this;
}

#ifdef WIN64
#pragma warning( pop )                  //AMD Port
#endif

SShaderPass::SShaderPass()
{
  m_RenderState = GS_DEPTHWRITE;

  m_PassFlags = 0;
  m_AlphaRef = ~0;

  m_VShader = NULL;
  m_PShader = NULL;
  m_GShader = NULL;
}

SShaderTechnique *SShaderItem::GetTechnique() const
{
  SShaderTechnique *pTech = NULL;
  int nTech = m_nTechnique;
  if (nTech < 0)
    nTech = 0;
  CShader *pSH = (CShader *)m_pShader;
  if (!pSH)
    return NULL;
  assert(m_nTechnique < 0 || pSH->m_HWTechniques.Num() == 0 || nTech < (int)pSH->m_HWTechniques.Num());
  if (nTech < (int)pSH->m_HWTechniques.Num())
    return pSH->m_HWTechniques[nTech];
  return NULL;
}

bool SShaderItem::IsMergable(SShaderItem& PrevSI)
{
  if (!PrevSI.m_pShader)
    return true;
  SRenderShaderResources *pRP = (SRenderShaderResources *)PrevSI.m_pShaderResources;
  SRenderShaderResources *pR = (SRenderShaderResources *)m_pShaderResources;
  if (pRP && pR)
  {
    if (pRP->m_AlphaRef != pR->m_AlphaRef)
      return false;
    if (pRP->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][3] != pR->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][3])
      return false;
    if (pRP->m_pDeformInfo != pR->m_pDeformInfo)
      return false;
    if (pRP->m_pDetailDecalInfo != pR->m_pDetailDecalInfo)
      return false;
    if ((pRP->m_ResFlags & MTL_FLAG_2SIDED) != (pR->m_ResFlags & MTL_FLAG_2SIDED))
      return false;
    if ((pRP->m_ResFlags & MTL_FLAG_NOSHADOW) != (pR->m_ResFlags & MTL_FLAG_NOSHADOW))
      return false;
    if(m_pShader->GetCull() != PrevSI.m_pShader->GetCull())
      return false;
  }
  return true;
}

uint32 SShaderTechnique::GetPreprocessFlags(CShader *pSH)
{
  uint32 i;
  uint32 nFlags = m_nPreprocessFlags;
  if (m_Flags & FHF_RE_FLARE)
    nFlags |= FSPR_CORONA;

  for (i=0; i<m_Passes.Num(); i++)
  {
    SShaderPass *pPass = &m_Passes[i];
    if (pPass->m_PShader)
      nFlags |= pPass->m_PShader->mfGetPreprocessFlags(this);
  }
  return nFlags;
}

SShaderTechnique *CShader::mfGetStartTechnique(int nTechnique)
{
  SShaderTechnique *pTech;
  if (m_HWTechniques.Num())
  {
    pTech = m_HWTechniques[0];
    if (nTechnique > 0)
    {
      assert(nTechnique < (int)m_HWTechniques.Num());
      if (nTechnique < (int)m_HWTechniques.Num())
        pTech = m_HWTechniques[nTechnique];
      else
        iLog->Log("ERROR: CShader::mfGetStartTechnique: Technique %d for shader '%s' is out of range", nTechnique, GetName());
    }
    else
    if (m_pSelectTech)
    {
      float fVarRef = m_pSelectTech->m_pCVarTech->GetFVal();
      int nRes = 0;
      switch(m_pSelectTech->m_eCompTech)
      {
        case eCF_NotEqual:
          nRes = fVarRef != m_pSelectTech->m_fValRefTech;
          break;
        case eCF_Equal:
          nRes = fVarRef == m_pSelectTech->m_fValRefTech;
          break;
        case eCF_Less:
          nRes = fVarRef < m_pSelectTech->m_fValRefTech;
          break;
        case eCF_Greater:
          nRes = fVarRef > m_pSelectTech->m_fValRefTech;
          break;
        case eCF_LEqual:
          nRes = fVarRef <= m_pSelectTech->m_fValRefTech;
          break;
        case eCF_GEqual:
          nRes = fVarRef >= m_pSelectTech->m_fValRefTech;
          break;
        default:
          assert(0);
      }
      pTech = m_HWTechniques[m_pSelectTech->m_nTech[nRes]];
    }
  }
  else
    pTech = NULL;

  return pTech;
}

#if !defined(XENON) && !defined(PS3) && !defined(NULL_RENDERER)
void CShader::mfFlushCache()
{
  uint32 n, m;

  mfFlushPendedShaders();

  if (SEmptyCombination::Combinations.size())
  {
    // Flush the cache before storing any empty combinations
    CHWShader::mfFlushPendedShadersWait(-1);
    for (m=0; m<SEmptyCombination::Combinations.size(); m++)
    {
      SEmptyCombination& Comb = SEmptyCombination::Combinations[m];
      Comb.pShader->mfStoreEmptyCombination(Comb);
    }
    SEmptyCombination::Combinations.clear();
  }

  for (m=0; m<m_HWTechniques.Num(); m++)
  {
    SShaderTechnique *pTech = m_HWTechniques[m];
    for (n=0; n<pTech->m_Passes.Num(); n++)
    {
      SShaderPass *pPass = &pTech->m_Passes[n];
      if (pPass->m_PShader)
        pPass->m_PShader->mfFlushCacheFile();
      if (pPass->m_VShader)
        pPass->m_VShader->mfFlushCacheFile();
    }
  }
}
#endif

void SRenderShaderResources::PostLoad(CShader *pSH)
{
  int i;
  m_nLastTexture = 0;
  for (i=0; i<EFTT_MAX; i++)
  {
    if (!m_Textures[i])
      continue;
    m_nLastTexture = i;
    SEfResTexture *pTex = m_Textures[i];
    pTex->m_TexModificator.m_UpdateFlags = 0;
    if (i != EFTT_DETAIL_OVERLAY) 
    {
      if (pTex->m_TexModificator.m_eUMoveType==ETMM_Pan && (pTex->m_TexModificator.m_UOscAmplitude==0 || pTex->m_TexModificator.m_UOscRate==0))
        pTex->m_TexModificator.m_eUMoveType = ETMM_NoChange;
      if (pTex->m_TexModificator.m_eVMoveType==ETMM_Pan && (pTex->m_TexModificator.m_VOscAmplitude==0 || pTex->m_TexModificator.m_VOscRate==0))
        pTex->m_TexModificator.m_eVMoveType = ETMM_NoChange;
      
      if (pTex->m_TexModificator.m_eUMoveType==ETMM_Fixed && pTex->m_TexModificator.m_UOscRate==0)
        pTex->m_TexModificator.m_eUMoveType = ETMM_NoChange;
      if (pTex->m_TexModificator.m_eVMoveType==ETMM_Fixed && pTex->m_TexModificator.m_VOscRate==0)
        pTex->m_TexModificator.m_eVMoveType = ETMM_NoChange;

      if (pTex->m_TexModificator.m_eUMoveType==ETMM_Constant && (pTex->m_TexModificator.m_UOscAmplitude==0 || pTex->m_TexModificator.m_UOscRate==0))
        pTex->m_TexModificator.m_eUMoveType = ETMM_NoChange;
      if (pTex->m_TexModificator.m_eVMoveType==ETMM_Constant && (pTex->m_TexModificator.m_VOscAmplitude==0 || pTex->m_TexModificator.m_VOscRate==0))
        pTex->m_TexModificator.m_eVMoveType = ETMM_NoChange;

      if (pTex->m_TexModificator.m_eUMoveType==ETMM_Stretch && (pTex->m_TexModificator.m_UOscAmplitude==0 || pTex->m_TexModificator.m_UOscRate==0))
        pTex->m_TexModificator.m_eUMoveType = ETMM_NoChange;
      if (pTex->m_TexModificator.m_eVMoveType==ETMM_Stretch && (pTex->m_TexModificator.m_VOscAmplitude==0 || pTex->m_TexModificator.m_VOscRate==0))
        pTex->m_TexModificator.m_eVMoveType = ETMM_NoChange;

      if (pTex->m_TexModificator.m_eUMoveType==ETMM_StretchRepeat && (pTex->m_TexModificator.m_UOscAmplitude==0 || pTex->m_TexModificator.m_UOscRate==0))
        pTex->m_TexModificator.m_eUMoveType = ETMM_NoChange;
      if (pTex->m_TexModificator.m_eVMoveType==ETMM_StretchRepeat && (pTex->m_TexModificator.m_VOscAmplitude==0 || pTex->m_TexModificator.m_VOscRate==0))
        pTex->m_TexModificator.m_eVMoveType = ETMM_NoChange;

      if (pTex->m_TexModificator.m_eTGType != ETG_Stream ||
          pTex->m_TexModificator.m_eUMoveType != ETMM_NoChange ||
          pTex->m_TexModificator.m_eVMoveType != ETMM_NoChange ||
          pTex->m_TexModificator.m_eRotType != ETMR_NoChange ||
          pTex->m_TexModificator.m_Tiling[0] != 1.0f ||
          pTex->m_TexModificator.m_Tiling[1] != 1.0f ||
          pTex->m_TexModificator.m_Offs[0] != 0.0f ||
          pTex->m_TexModificator.m_Offs[1] != 0.0f)
      {
        m_ResFlags |= MTL_FLAG_NOTINSTANCED;
      }
    }
  }
  if (pSH && (pSH->m_Flags & EF_SKY))
  {
    if (m_Textures[EFTT_DIFFUSE])
    {
      char sky[128];
      char path[1024];
      strcpy(sky, m_Textures[EFTT_DIFFUSE]->m_Name.c_str());
      int size = strlen(sky);
      const char *ext = fpGetExtension(sky);
      while (sky[size] != '_')
      {
        size--;
        if (!size)
          break;
      }
      sky[size] = 0;
      if (size)
      {
        m_pSky = new SSkyInfo;
        sprintf(path, "%s_12%s", sky, ext);
        m_pSky->m_SkyBox[0] = CTexture::ForName(path, 0, eTF_Unknown);
        sprintf(path, "%s_34%s", sky, ext);
        m_pSky->m_SkyBox[1] = CTexture::ForName(path, 0, eTF_Unknown);
        sprintf(path, "%s_5%s", sky, ext);
        m_pSky->m_SkyBox[2] = CTexture::ForName(path, 0, eTF_Unknown);
      }
    }
  }
#ifdef USE_PER_MATERIAL_PARAMS
  UpdateConstants(pSH);
#endif
}

CTexture *CShader::mfFindBaseTexture(TArray<SShaderPass>& Passes, int *nPass, int *nTU)
{
  return NULL;
}

ITexture *CShader::GetBaseTexture(int *nPass, int *nTU)
{
  for (uint32 i=0; i<m_HWTechniques.Num(); i++)
  {
    SShaderTechnique *hw = m_HWTechniques[i];
    CTexture *tx = mfFindBaseTexture(hw->m_Passes, nPass, nTU);
    if (tx)
      return tx;
  }
  if (nPass)
    *nPass = -1;
  if (nTU)
    *nTU = -1;
  return NULL;
}

unsigned int CShader::GetUsedTextureTypes (void)
{
  uint32 nMask = 0xffffffff;

  return nMask;
}

//================================================================================

void CShaderMan::mfReleaseShaders ()
{
  CCryName Name = CShader::mfGetClassName();
  SResourceContainer *pRL = CBaseResource::GetResourcesForClass(Name);
  if (pRL)
  {
    int n = 0;
    ResourcesMapItor itor;
    for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); )
    {
      CShader *sh = (CShader *)itor->second;
      itor++;
      if (!sh)
        continue;
      if (CRenderer::CV_r_printmemoryleaks && !(sh->m_Flags & EF_SYSTEM))
        iLog->Log("Warning: CShaderMan::mfClearAll: Shader %s was not deleted (%d)", sh->GetName(), sh->GetRefCounter());
      if (!sh->m_ShaderGenParams)
      {
        SAFE_RELEASE_FORCE(sh);
      }
      else
      {
        SAFE_RELEASE(sh);
      }
      n++;
    }
  }
}

void CShaderMan::ShutDown(void)
{
  uint32 i;
  
  gRenDev->m_cEF.m_Bin.InvalidateCache();

  if (CRenderer::CV_r_releaseallresourcesonexit)
  {
    if (gRenDev->m_cEF.m_bInitialized)
    {
      mfReleaseShaders();
    }

    for (i=0; i<CShader::m_ShaderResources_known.Num(); i++)
    {
      SRenderShaderResources *pSR = CShader::m_ShaderResources_known[i];
      if (!pSR)
        continue;
      if (i)
      {
        if (CRenderer::CV_r_printmemoryleaks)
          iLog->Log("Warning: CShaderMan::mfClearAll: Shader resource 0x%x was not deleted", (uintptr_t)pSR);
      }
      delete pSR;
    }
    CShader::m_ShaderResources_known.Free();
  }

  m_AliasNames.clear();
  m_CustomAliasNames.clear();
  m_ShaderNames.clear();
  
  std::for_each(m_pShadersGlobalFlags.begin(), m_pShadersGlobalFlags.end(), SShaderMapNameFlagsContainerDelete());  
  m_pShadersGlobalFlags.clear(); 

  m_pShaderCommonGlobalFlag.clear();
  m_pShadersGlobalFlags.clear();


  CCryName Name;

  CHWShader::mfBeginFrame(10000);

  SAFE_DELETE(m_pGlobalExt);

  for (i=0; i<CLightStyle::m_LStyles.Num(); i++)
  {
    delete CLightStyle::m_LStyles[i];
  }
  CLightStyle::m_LStyles.Free();
  m_SGC.clear();
  m_ShaderCacheCombinations[0].clear();
  m_ShaderCacheCombinations[1].clear();
  SAFE_DELETE(m_pGlobalExt);
  mfCloseShadersCache(0);
  mfCloseShadersCache(1);
  m_GR.ShutDown();

  gRenDev->m_cEF.m_bInitialized = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan:: mfCreateCommonGlobalFlags( const char *szName )
{
  // Create globals.txt

  assert( szName );  
  uint32 nCurrMaskCount = 0;
  const char *pszShaderExtPath = "Shaders/";

  struct _finddata_t fileinfo;
  intptr_t handle;
  char dirn[256];
  strcpy(dirn, pszShaderExtPath ) ;
  strcat(dirn, "*.*");

  handle = gEnv->pCryPak->FindFirst (dirn, &fileinfo);
  if (handle == -1)// failed search
    return; 
    
  do
  {
    // Scan for extension script files - add common flags names into globals list

    if (fileinfo.name[0] == '.' || fileinfo.attrib & _A_SUBDIR)
      continue;

    const char *pszExt = PathUtil::GetExt( fileinfo.name );
    if( stricmp( pszExt, "ext") )
      continue;
    

    char pszFileName[256];
    sprintf(pszFileName, "%s%s", pszShaderExtPath, fileinfo.name);
        
    FILE *fp = gEnv->pCryPak->FOpen(pszFileName, "r");
    if (fp)
    {
      gEnv->pCryPak->FSeek(fp, 0, SEEK_END);
      int len = gEnv->pCryPak->FTell(fp);
      char *buf = new char [len+1];
      gEnv->pCryPak->FSeek(fp, 0, SEEK_SET);
      len = gEnv->pCryPak->FRead(buf, len, fp);
      gEnv->pCryPak->FClose(fp);
      buf[len] = 0;
      
      // Check if global flags are common
      char *pCurrOffset = strstr( buf, "UsesCommonGlobalFlags" );
      if( pCurrOffset )
      {           
        // add shader to list
        string pszShaderName = PathUtil::GetFileName( fileinfo.name );
				pszShaderName.MakeUpper();
        m_pShadersRemapList += string("%") + pszShaderName;

        while( pCurrOffset = strstr( pCurrOffset, "Name" ) )
        {
          pCurrOffset += 4;          
          char dummy[256] = "\n";
          char name[256] = "\n";
          int res = sscanf(pCurrOffset, "%s %s", dummy, name);      // Get flag name..
          assert(res);

          string pszNameFlag = name;
          int nSzSize = pszNameFlag.size();
          pszNameFlag.MakeUpper();

          MapNameFlagsItor pCheck = m_pShaderCommonGlobalFlag.find( pszNameFlag );
          if( pCheck == m_pShaderCommonGlobalFlag.end() )
          {
            m_pShaderCommonGlobalFlag.insert(  MapNameFlagsItor::value_type( pszNameFlag, 0 ) );
            if( ++nCurrMaskCount >= 64 )  // sanity check
            {
              assert(0);
              break;
            }
          }
        }
      }

      SAFE_DELETE_ARRAY(buf);
    }

    if( nCurrMaskCount >= 64 )  
      break;

  } while (gEnv->pCryPak->FindNext(handle, &fileinfo) != -1);

  gEnv->pCryPak->FindClose (handle);       

  if( nCurrMaskCount >= 64 )
    iLog->Log("ERROR: CShaderMan::mfCreateCommonGlobalFlags: too many common global flags");

  uint64 nCurrGenMask = 0;
  MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
  MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();    
  for( ; pIter != pEnd ; ++pIter, ++nCurrGenMask )
  {
    // Set mask value
    pIter->second = ((uint64)1) << nCurrGenMask;   
  }

  mfRemapCommonGlobalFlagsWithLegacy();
  mfSaveCommonGlobalFlagsToDisk( szName, nCurrMaskCount );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan::mfSaveCommonGlobalFlagsToDisk( const char *szName, uint32 nMaskCount )
{  
  // Write all flags
  assert( nMaskCount );

  FILE *fp = gEnv->pCryPak->FOpen(szName, "w");
  if (fp)
  {   
    gEnv->pCryPak->FPrintf(fp,"FX_CACHE_VER %f\n", FX_CACHE_VER);    
    gEnv->pCryPak->FPrintf(fp,"%s\n\n", m_pShadersRemapList.c_str());    
    
    MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
    MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();    
    
    for( ; pIter != pEnd ; ++pIter )
    {
      gEnv->pCryPak->FPrintf(fp,"%s "
#if defined(__GNUC__)
        "%llx\n"
#else
        "%I64x\n"
#endif
        , (pIter->first).c_str(), pIter->second );
    }

    gEnv->pCryPak->FClose(fp);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan::mfInitCommonGlobalFlagsLegacyFix(void)
{                       
  // Store original values since common material flags where introduced
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%ALLOW_POM"                 , (uint64)0x1 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%ALPHAGLOW"                 , (uint64)0x2 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%ALPHAMASK_DETAILMAP"       , (uint64)0x4 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%ANISO_SPECULAR"            , (uint64)0x8 ) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%BILINEAR_FP16"             , (uint64)0x10 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%BUMP_DIFFUSE"              , (uint64)0x20 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%CHARACTER_DECAL"           , (uint64)0x40 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%CUSTOM_DIFFUSE"            , (uint64)0x80 ) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%CUSTOM_MAP"                , (uint64)0x100 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%CUSTOM_SECONDARY_MAP"      , (uint64)0x200 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%CUSTOM_SPECULAR"           , (uint64)0x400 ) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%DECAL"                     , (uint64)0x800 ) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%DETAIL_BENDING"            , (uint64)0x1000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%DETAIL_BUMP_MAPPING"       , (uint64)0x2000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%DISABLE_RAIN_PASS"         , (uint64)0x4000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%DYN_BRANCHING"             , (uint64)0x8000) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%ENVIRONMENT_MAP"           , (uint64)0x10000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%EYE_OVERLAY"               , (uint64)0x20000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%GLOSS_DIFFUSEALPHA"        , (uint64)0x40000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%GLOSS_MAP"                 , (uint64)0x80000) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%GRADIENT_COLORING"         , (uint64)0x100000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%GRASS"                     , (uint64)0x200000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%IRIS"                      , (uint64)0x400000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%LEAVES"                    , (uint64)0x800000) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%NANOSUIT_EFFECTS"          , (uint64)0x1000000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%OFFSETBUMPMAPPING"         , (uint64)0x2000000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%OPACITY_MAP"               , (uint64)0x4000000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%PARALLAX_OCCLUSION_MAPPING", (uint64)0x8000000) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%REALTIME_MIRROR_REFLECTION", (uint64)0x10000000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%REFRACTION_MAP"            , (uint64)0x20000000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%RIM_LIGHTING"              , (uint64)0x40000000) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%SPECULARPOW_GLOSSALPHA"    , (uint64)0x80000000) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%STAT_BRANCHING"            , (uint64)0x100000000ULL) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%TEMP_TERRAIN"              , (uint64)0x200000000ULL) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%TEMP_VEGETATION"           , (uint64)0x400000000ULL) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%TERRAINHEIGHTADAPTION"     , (uint64)0x800000000ULL) );

  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%TWO_SIDED_SORTING"         , (uint64)0x1000000000ULL) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%VERTCOLORS"                , (uint64)0x2000000000ULL) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%WIND_BENDING"              , (uint64)0x4000000000ULL) );
  m_pSCGFlagLegacyFix.insert( MapNameFlagsItor::value_type( "%WRINKLE_BLENDING"          , (uint64)0x8000000000ULL) );  
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool CShaderMan::mfRemapCommonGlobalFlagsWithLegacy(void)
{
  MapNameFlagsItor pFixIter = m_pSCGFlagLegacyFix.begin();
  MapNameFlagsItor pFixEnd = m_pSCGFlagLegacyFix.end();    

  MapNameFlagsItor pCommonGlobalsEnd = m_pShaderCommonGlobalFlag.end();    

  bool bRemaped = false;
  for( ; pFixIter != pFixEnd ; ++pFixIter )
  {
    MapNameFlagsItor pFoundMatch = m_pShaderCommonGlobalFlag.find( pFixIter->first );
    if( pFoundMatch != pCommonGlobalsEnd )
    {      
      // Found a match, replace value
      uint64 nRemapedMask = pFixIter->second;
      uint64 nOldMask = pFoundMatch->second;
      pFoundMatch->second = nRemapedMask;

      // Search for duplicates and swap with old mask
      MapNameFlagsItor pCommonGlobalsIter = m_pShaderCommonGlobalFlag.begin();    
      uint64 test = (uint64)0x10;
      for( ; pCommonGlobalsIter != pCommonGlobalsEnd ; ++pCommonGlobalsIter )
      {
        if( pFoundMatch != pCommonGlobalsIter && pCommonGlobalsIter->second == nRemapedMask )
        {
          uint64 nPrev = pCommonGlobalsIter->second;
          pCommonGlobalsIter->second = nOldMask;
          bRemaped = true;
          break;
        }
      }
    }
  }

  // Create existing flags mask
  MapNameFlagsItor pIter = m_pShaderCommonGlobalFlag.begin();
  MapNameFlagsItor pEnd = m_pShaderCommonGlobalFlag.end();	
  m_nSGFlagsFix = 0;
  for ( ; pIter != pEnd; ++pIter )
  {	
    m_nSGFlagsFix |= (pIter->second);
  }

  return bRemaped;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CShaderMan::mfInitCommonGlobalFlags(void)
{  
  mfInitCommonGlobalFlagsLegacyFix();
  
  string pszGlobalsPath = CRenderer::CV_r_shadersuserfolder? string( m_szUserPath) + string( "Shaders/Cache/globals.txt" ) : string( "Shaders/Cache/globals.txt");
  FILE *fp = gEnv->pCryPak->FOpen(pszGlobalsPath.c_str(), "r", ICryPak::FOPEN_HINT_QUIET);    
  if( fp )
  {
    char str[256] = "\n";
    char name[128] = "\n";

    gEnv->pCryPak->FGets(str, 256, fp);
    if( strstr( str, "FX_CACHE_VER" ) )
    {
      float fCacheVer = 0.0f;
      int res = sscanf(str, "%s %f", name, &fCacheVer );
			assert(res);
      if( !stricmp(name, "FX_CACHE_VER") && fabs(FX_CACHE_VER - fCacheVer) >= 0.01f ) 
      {
        // re-create common global flags (shader cache bumped)
        mfCreateCommonGlobalFlags( pszGlobalsPath.c_str() );
        
        gEnv->pCryPak->FClose(fp);
        return;
      }
    }
    
    // get shader that need remapping slist
    gEnv->pCryPak->FGets(str, 256, fp);    
    m_pShadersRemapList = str;

    uint32 nCurrMaskCount = 0;
    while (!gEnv->pCryPak->FEof(fp))
    {
      uint64 nGenMask = 0;      
      gEnv->pCryPak->FGets(str, 256, fp);
            
      if ( sscanf(str, "%s "
#if defined(__GNUC__)
        "%llx"
#else
        "%I64x"
#endif
        , name, &nGenMask) > 1 )
      {
        m_pShaderCommonGlobalFlag.insert(  MapNameFlagsItor::value_type( name, nGenMask ) );
        nCurrMaskCount ++;
      }
    }

    gEnv->pCryPak->FClose(fp);
    
    if( mfRemapCommonGlobalFlagsWithLegacy() )
      mfSaveCommonGlobalFlagsToDisk( pszGlobalsPath.c_str(), nCurrMaskCount );

    return;
  }

  // create common global flags - not existing globals.txt
  mfCreateCommonGlobalFlags( pszGlobalsPath.c_str() );
}

void CShaderMan::mfInitGlobal (void) 
{
  SAFE_DELETE(m_pGlobalExt);
  SShaderGen *pShGen = mfCreateShaderGenInfo("Shaders/RunTime.ext");
  m_pGlobalExt = pShGen;
  if (pShGen)
  {
    uint32 i;

    for (i=0; i<pShGen->m_BitMask.Num(); i++)
    {
      SShaderGenBit *gb = pShGen->m_BitMask[i];
      if (!gb)
        continue;
      if (gb->m_ParamName == "%_RT_FOG")
        g_HWSR_MaskBit[HWSR_FOG] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_AMBIENT")
        g_HWSR_MaskBit[HWSR_AMBIENT] = gb->m_Mask;
      else
			if (gb->m_ParamName == "%_RT_DEFERRED_SHADING")
				g_HWSR_MaskBit[HWSR_DEFERRED_SHADING] = gb->m_Mask;
			else
      if (gb->m_ParamName == "%_RT_SRGB")
        g_HWSR_MaskBit[HWSR_RT_SRGB] = gb->m_Mask;
      else      
      if (gb->m_ParamName == "%_RT_HDR_ENCODE")
        g_HWSR_MaskBit[HWSR_HDR_ENCODE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_ALPHATEST")
        g_HWSR_MaskBit[HWSR_ALPHATEST] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_HDR_MODE")
        g_HWSR_MaskBit[HWSR_HDR_MODE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_FSAA_QUALITY")
        g_HWSR_MaskBit[HWSR_FSAA_QUALITY] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_FSAA")
        g_HWSR_MaskBit[HWSR_FSAA] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_NEAREST")
        g_HWSR_MaskBit[HWSR_NEAREST] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SHADOW_MIXED_MAP_G16R16")
        g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_HW_PCF_COMPARE")
        g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SAMPLE0")
        g_HWSR_MaskBit[HWSR_SAMPLE0] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SAMPLE1")
        g_HWSR_MaskBit[HWSR_SAMPLE1] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SAMPLE2")
        g_HWSR_MaskBit[HWSR_SAMPLE2] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SAMPLE3")
        g_HWSR_MaskBit[HWSR_SAMPLE3] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SHADOW_FILTER")
        g_HWSR_MaskBit[HWSR_SHADOW_FILTER] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_ALPHABLEND")
        g_HWSR_MaskBit[HWSR_ALPHABLEND] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_QUALITY")
        g_HWSR_MaskBit[HWSR_QUALITY] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_QUALITY1")
        g_HWSR_MaskBit[HWSR_QUALITY1] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_CAMERA_SPACE")
        g_HWSR_MaskBit[HWSR_CAMERA_SPACE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_WORLD_SPACE")
        g_HWSR_MaskBit[HWSR_WORLD_SPACE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_INSTANCING_ROT")
        g_HWSR_MaskBit[HWSR_INSTANCING_ROT] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_INSTANCING_ATTR")
        g_HWSR_MaskBit[HWSR_INSTANCING_ATTR] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_INSTANCING_CONST")
        g_HWSR_MaskBit[HWSR_INSTANCING_CONST] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_NOZPASS")
        g_HWSR_MaskBit[HWSR_NOZPASS] = gb->m_Mask;
			else
			if (gb->m_ParamName == "%_RT_ZPREPASS")
				g_HWSR_MaskBit[HWSR_ZPREPASS] = gb->m_Mask;
			else
      if (gb->m_ParamName == "%_RT_SKYLIGHT_BASED_FOG")
        g_HWSR_MaskBit[HWSR_SKYLIGHT_BASED_FOG] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SHAPEDEFORM")
        g_HWSR_MaskBit[HWSR_SHAPEDEFORM] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_MORPHTARGET")
        g_HWSR_MaskBit[HWSR_MORPHTARGET] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_OBJ_IDENTITY")
        g_HWSR_MaskBit[HWSR_OBJ_IDENTITY] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SPHERICAL")
        g_HWSR_MaskBit[HWSR_SPHERICAL] = gb->m_Mask;
			else
			if (gb->m_ParamName == "%_RT_SKELETON_SSD")
				g_HWSR_MaskBit[HWSR_SKELETON_SSD] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_DISSOLVE")
        g_HWSR_MaskBit[HWSR_DISSOLVE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SOFT_PARTICLE")
        g_HWSR_MaskBit[HWSR_SOFT_PARTICLE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_OCEAN_PARTICLE")
        g_HWSR_MaskBit[HWSR_OCEAN_PARTICLE] = gb->m_Mask;
      else        
      if (gb->m_ParamName == "%_RT_LIGHT_TEX_PROJ")
        g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SHADOW_JITTERING")
        g_HWSR_MaskBit[HWSR_SHADOW_JITTERING] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_VARIANCE_SM")
        g_HWSR_MaskBit[HWSR_VARIANCE_SM] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_PARTICLE")
        g_HWSR_MaskBit[HWSR_PARTICLE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_BLEND_WITH_TERRAIN_COLOR")
        g_HWSR_MaskBit[HWSR_BLEND_WITH_TERRAIN_COLOR] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_AMBIENT_OCCLUSION")
        g_HWSR_MaskBit[HWSR_AMBIENT_OCCLUSION] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_GSM_COMBINED")
        g_HWSR_MaskBit[HWSR_GSM_COMBINED] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SCATTERSHADE")
        g_HWSR_MaskBit[HWSR_SCATTERSHADE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_DEBUG0")
        g_HWSR_MaskBit[HWSR_DEBUG0] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_DEBUG1")
        g_HWSR_MaskBit[HWSR_DEBUG1] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_DEBUG2")
        g_HWSR_MaskBit[HWSR_DEBUG2] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_DEBUG3")
        g_HWSR_MaskBit[HWSR_DEBUG3] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_POINT_LIGHT")
        g_HWSR_MaskBit[HWSR_POINT_LIGHT] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_TEX_ARR_SAMPLE")
        g_HWSR_MaskBit[HWSR_TEX_ARR_SAMPLE] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_CUBEMAP0")
        g_HWSR_MaskBit[HWSR_CUBEMAP0] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_CUBEMAP1")
        g_HWSR_MaskBit[HWSR_CUBEMAP1] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_CUBEMAP2")
        g_HWSR_MaskBit[HWSR_CUBEMAP2] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_CUBEMAP3")
        g_HWSR_MaskBit[HWSR_CUBEMAP3] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_VEGETATION")
        g_HWSR_MaskBit[HWSR_VEGETATION] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_DECAL_TEXGEN_2D")
        g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_DECAL_TEXGEN_3D")
        g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_3D] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SAMPLE4")
        g_HWSR_MaskBit[HWSR_SAMPLE4] = gb->m_Mask;
      else
      if (gb->m_ParamName == "%_RT_SHADER_LOD")
        g_HWSR_MaskBit[HWSR_SHADER_LOD] = gb->m_Mask;
			else 
				assert(0);
    }
  }
}

void CShaderMan::mfInit (void) 
{
  s_cNameHEAD = CCryNameCRC("HEAD");

  CTexture::Init();

  if (!m_bInitialized)
  {
    m_ShadersPath = "Shaders/HWScripts/";
    m_ShadersMergeCachePath = "Shaders/MergeCache/";
#ifdef OPENGL
    m_ShadersCache = "Cache/Shaders/";
#elif defined (DIRECT3D9)
#ifndef XENON
    m_ShadersCache = "Shaders/Cache/D3D9/";


#endif
#elif defined(DIRECT3D10)
    m_ShadersCache = "Shaders/Cache/D3D10/";
#elif defined(LINUX32)
    m_ShadersCache = "Shaders/Cache/LINUX32/";
#elif defined(LINUX64)
    m_ShadersCache = "Shaders/Cache/LINUX64/";
#else
    m_ShadersCache = "Shaders/Cache/";
#endif
    m_szUserPath = "%USER%/";

		fxParserInit();
    CParserBin::Init();

    mfInitShadersList(&m_ShaderNames);

    //CShader::m_Shaders_known.Alloc(MAX_SHADERS);
    //memset(&CShader::m_Shaders_known[0], 0, sizeof(CShader *)*MAX_SHADERS);

    m_AliasNames.clear();
    mfInitGlobal();

		// Generate/or load globals.txt - if not existing or shader cache version bumped
		mfInitCommonGlobalFlags();

		mfInitShadersCache(false, NULL, NULL, 0);
    if (CRenderer::CV_r_shaderspreactivate == 2)
      mfInitShadersCache(false, NULL, NULL, 1);

#if !defined(NULL_RENDERER)
    //FILE *fp = fxopen("Shaders/Aliases.txt", "r");
    FILE *fp = gEnv->pCryPak->FOpen("Shaders/Aliases.txt", "r");
    if (fp)
    {
      while (!gEnv->pCryPak->FEof(fp))
      {
        char name[128];
        char alias[128];
        char str[256];
        gEnv->pCryPak->FGets(str, 256, fp);
        int n;
        uint64 nGenMask;



				if ((n=sscanf(str, "%s %s %I64x", alias, name, &nGenMask)) >= 2)

        {
          SNameAlias na;
          na.m_Alias = CCryName(alias);
          na.m_Name = CCryName(name);
          if (n == 3)
            na.m_nGenMask = nGenMask;
          else
            na.m_nGenMask = 0;
          m_AliasNames.push_back(na);
        }
      }
      gEnv->pCryPak->FClose(fp);
    }

    int nGPU = gRenDev->GetFeatures() & RFT_HW_MASK;

    // load CustomAliases (by wat)
    m_CustomAliasNames.clear();
    fp = gEnv->pCryPak->FOpen("Shaders/CustomAliases.txt", "r");
    if (fp)
    {
      char arg[3][256];
      bool bCond = true;
      while (!gEnv->pCryPak->FEof(fp))
      {
        char str[256];
        gEnv->pCryPak->FGets(str, 256, fp);

        char * p = strchr(str, '=');
        if(p)
        {
          memmove(p+2,p,strlen(p)+1);
          str[p-str] = ' '; str[p-str+1] = '='; str[p-str+2] = ' ';
        }

        int nArgs = sscanf(str, "%s %s %s", arg[0], arg[1], arg[2]);
        // parser comments
        for(int i = 0; i<nArgs; i++)
          if(arg[i][0] == ';')
          {
            nArgs = i;
            break;
          }
        // parser aliases
        if(nArgs == 2 && bCond)
        {
          SNameAlias na;
          na.m_Alias = CCryName(arg[0]);
          na.m_Name = CCryName(arg[1]);
          //if(m_CustomAliasNames.Find(na) == -1)
          m_CustomAliasNames.push_back(na);
        }
        // parser conditions vars.
        else if(nArgs == 3 && arg[1][0] == '=')
        {
          ICVar * pVar = iConsole->GetCVar(arg[0]);
          if(pVar)
          {
            if( pVar->GetType()== CVAR_INT && pVar->GetIVal() != atoi(arg[2]))
              bCond = false;
            if( pVar->GetType()== CVAR_FLOAT && pVar->GetFVal() != float(atof(arg[2])))
              bCond = false;
          }
          else
          if(!strnicmp(arg[0], "GPU", 3))
          {
            if(!strnicmp(arg[2], "NV1X", 4) && nGPU != RFT_HW_GF2)
              bCond = false;
            else
            if(!strnicmp(arg[2], "NV2X", 4) && nGPU != RFT_HW_GF3)
              bCond = false;
            else
            if(!strnicmp(arg[2], "ATI", 4) && nGPU != RFT_HW_ATI)		// was R300
              bCond = false;
            else
            if(!strnicmp(arg[2], "NV4X", 4) && nGPU != RFT_HW_NV4X)
              bCond = false;
          }
        }
        else if(arg[0][0] == '}')
          bCond = true;
      }
      gEnv->pCryPak->FClose(fp);
    }
    for (uint32 i=0; i<m_CustomAliasNames.size(); i++)
    {
      CCryName nm = m_CustomAliasNames[i].m_Name;
      bool bChanged = false;
      for (uint32 j=i+1; j<m_CustomAliasNames.size(); j++)
      {
        if (m_CustomAliasNames[j].m_Alias == nm)
        {
          m_CustomAliasNames[i].m_Name = m_CustomAliasNames[j].m_Name;
          bChanged = true;
        }
      }
    }


    /* Old version:
    fp = gEnv->pCryPak->FOpen("Shaders/CustomAliases.txt", "r");
    if (fp)
    {
      while (!gEnv->pCryPak->FEof(fp))
      {
        char name[128];
        char alias[128];
        char str[256];
        gEnv->pCryPak->FGets(str, 256, fp);
        if (sscanf(str, "%s %s", alias, name) == 2)
        {
          SNameAlias na;
          na.m_Alias = CCryName(alias, eFN_Add);
          na.m_Name = CCryName(name, eFN_Add);
          m_CustomAliasNames.AddElem(na);
        }
      }
      gEnv->pCryPak->FClose(fp);
    }
    */

#if (defined(WIN32) || defined(WIN64))
    // make sure we can write to the shader cache
    if(!CheckAllFilesAreWritable((string(m_ShadersCache) + "cgpshaders").c_str())
      || !CheckAllFilesAreWritable((string(m_ShadersCache) + "cgvshaders").c_str()))
    {
// message box causes problems in fullscreen 
//			MessageBox(0,"WARNING: Shader cache cannot be updated\n\n"
//				"files are write protected / media is read only / windows user setting don't allow file changes","CryEngine",MB_ICONEXCLAMATION|MB_OK);
      gEnv->pLog->LogError("ERROR: Shader cache cannot be updated (files are write protected / media is read only / windows user setting don't allow file changes)");
    }
#endif	// WIN

#endif //NULL_RENDERER
    mfSetDefaults();

#if !defined(XENON) && !defined(PS3)
    m_GR.Init();
#endif
    //mfPrecacheShaders(NULL);

    m_bInitialized = true;
  }
  //mfPrecacheShaders();
}

void CShaderMan::ParseShaderProfile(char *scr, SShaderProfile *pr)
{
  char* name;
  long cmd;
  char *params;
  char *data;

  enum {eUseNormalAlpha=1};
  static STokenDesc commands[] =
  {
    {eUseNormalAlpha, "UseNormalAlpha"},

    {0,0}
  };

  while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
  {
    data = NULL;
    if (name)
      data = name;
    else
    if (params)
      data = params;

    switch (cmd)
    {
      case eUseNormalAlpha:
        pr->m_nShaderProfileFlags |= SPF_LOADNORMALALPHA;
        break;
    }
  }
}

//==========================================================================================

bool CShaderMan::ParseFSAADevices(char *scr, SDevID *pDev)
{
  char* name;
  long cmd;
  char *params;
  char *data;

  int bRes = 0;

  enum {eMinID=1, eMaxID};
  static STokenDesc commands[] =
  {
    {eMinID, "MinID"},
    {eMaxID, "MaxID"},

    {0,0}
  };

  SMinMax mm;

  while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
  {
    data = NULL;
    if (name)
      data = name;
    else
    if (params)
      data = params;

    switch (cmd)
    {
    case eMinID:
      {
        mm.nMin = shGetInt(data);
        bRes |= 1;
      }
      break;
    case eMaxID:
      {
        mm.nMax = shGetInt(data);
        bRes |= 2;
      }
      break;
    }
  }
  if (bRes == 3)
    pDev->DevMinMax.push_back(mm);
  return (bRes == 3);
}

bool CShaderMan::ParseFSAADevGroup(char *scr, SDevID *pDev)
{
  char* name;
  long cmd;
  char *params;
  char *data;
  int bRes = 0;

  enum {eVendorID=1, eDevices};
  static STokenDesc commands[] =
  {
    {eVendorID, "VendorID"},
    {eDevices, "Devices"},

    {0,0}
  };

  while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
  {
    data = NULL;
    if (name)
      data = name;
    else
    if (params)
      data = params;

    switch (cmd)
    {
    case eVendorID:
      {
        pDev->nVendorID = shGetInt(data);
        bRes |= 1;
      }
      break;
    case eDevices:
      {
        if (ParseFSAADevices(params, pDev))
          bRes |= 2;
      }
      break;
    }
  }
  return (bRes == 3);
}

bool CShaderMan::ParseFSAAMode(char *scr, SMSAAMode *pMSAA)
{
  char* name;
  long cmd;
  char *params;
  char *data;
  int bRes = 0;

  enum {eSamples=1, eQuality, eDesc};
  static STokenDesc commands[] =
  {
    {eSamples, "Samples"},
    {eQuality, "Quality"},
    {eDesc, "Desc"},

    {0,0}
  };

  while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
  {
    data = NULL;
    if (name)
      data = name;
    else
    if (params)
      data = params;

    switch (cmd)
    {
    case eSamples:
      {
        pMSAA->nSamples = shGetInt(data);
        bRes |= 1;
      }
      break;
    case eQuality:
      {
        pMSAA->nQuality = shGetInt(data);
        bRes |= 2;
      }
      break;
    case eDesc:
      {
        pMSAA->szDescr = data;
        bRes |= 4;
      }
      break;
    }
  }
  return (bRes == 7);
}

bool CShaderMan::ParseFSAAProfile(char *scr, SMSAAProfile *pMSAA)
{
  char* name;
  long cmd;
  char *params;
  char *data;
  int bRes = 0;

  enum {eDeviceGroup=1, eMode};
  static STokenDesc commands[] =
  {
    {eDeviceGroup, "DeviceGroup"},
    {eMode, "Mode"},

    {0,0}
  };

  while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
  {
    data = NULL;
    if (name)
      data = name;
    else
    if (params)
      data = params;

    switch (cmd)
    {
    case eDeviceGroup:
      {
        string szName = data;
        pMSAA->pDeviceGroup = NULL;
        for (int i=0; i<m_FSAADevGroups.size(); i++)
        {
          if (m_FSAADevGroups[i]->szName == szName)
          {
            pMSAA->pDeviceGroup = m_FSAADevGroups[i];
            bRes |= 1;
            break;
          }
        }
      }
      break;
    case eMode:
      {
        SMSAAMode mm;
        if (ParseFSAAMode(params, &mm))
        {
          pMSAA->Modes.push_back(mm);
          bRes |= 2;
        }
      }
      break;
    }
  }
  return (bRes == 3);
}

void CShaderMan::ParseFSAAProfiles()
{
  char* name;
  long cmd;
  char *params;
  char *data;

  char *scr = NULL;
#ifdef DIRECT3D10
  FILE *fp = gEnv->pCryPak->FOpen("Config/FSAAProfilesDX11.txt", "rb");
#else
  FILE *fp = gEnv->pCryPak->FOpen("Config/FSAAProfiles.txt", "rb");
#endif
  if (fp)
  {
    gEnv->pCryPak->FSeek(fp, 0, SEEK_END);
    int ln = gEnv->pCryPak->FTell(fp);
    scr = new char [ln+1];
    if (scr)
    {
      scr[ln] = 0;
      gEnv->pCryPak->FSeek(fp, 0, SEEK_SET);
      gEnv->pCryPak->FRead(scr, ln, fp);
      gEnv->pCryPak->FClose(fp);
    }
  }
  char *pScr = scr;

  enum {eVersion=1, eDeviceGroupID, eFSAAProfile};
  static STokenDesc commands[] =
  {
    {eVersion, "Version"},
    {eDeviceGroupID, "DeviceGroupID"},
    {eFSAAProfile, "FSAAProfile"},

    {0,0}
  };

  while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
  {
    data = NULL;
    if (name)
      data = name;
    else
    if (params)
      data = params;

    switch (cmd)
    {
    case eDeviceGroupID:
      {
        SDevID *pDev = new SDevID;
        pDev->szName = name;
        if (ParseFSAADevGroup(params, pDev))
          m_FSAADevGroups.push_back(pDev);
      }
      break;
    case eFSAAProfile:
      {
        SMSAAProfile MSAA;
        MSAA.szName = name;
        if (ParseFSAAProfile(params, &MSAA))
          m_FSAAModes.push_back(MSAA);
      }
      break;
    case eVersion:
      break;
    }
  }
  SAFE_DELETE_ARRAY(pScr);
}

void CShaderMan::ParseShaderProfiles()
{
  int i;

  for (i=0; i<eSQ_Max; i++)
  {
    m_ShaderFixedProfiles[i].m_iShaderProfileQuality = i;
    m_ShaderFixedProfiles[i].m_nShaderProfileFlags = 0;
  }

  char* name;
  long cmd;
  char *params;
  char *data;

  enum {eProfile=1, eVersion};
  static STokenDesc commands[] =
  {
    {eProfile, "Profile"},
    {eVersion, "Version"},

    {0,0}
  };

  char *scr = NULL;
  FILE *fp = gEnv->pCryPak->FOpen("Shaders/ShaderProfiles.txt", "rb");
  if (fp)
  {
    gEnv->pCryPak->FSeek(fp, 0, SEEK_END);
    int ln = gEnv->pCryPak->FTell(fp);
    scr = new char [ln+1];
    if (scr)
    {
      scr[ln] = 0;
      gEnv->pCryPak->FSeek(fp, 0, SEEK_SET);
      gEnv->pCryPak->FRead(scr, ln, fp);
      gEnv->pCryPak->FClose(fp);
    }
  }
  char *pScr = scr;
  if (scr)
  {
    while ((cmd = shGetObject (&scr, commands, &name, &params)) > 0)
    {
      data = NULL;
      if (name)
        data = name;
      else
      if (params)
        data = params;

      switch (cmd)
      {
      case eProfile:
        {
          SShaderProfile *pr = NULL;
          if (!stricmp(name, "Low"))
            pr = &m_ShaderFixedProfiles[eSQ_Low];
          else
          {
            pr = &m_ShaderFixedProfiles[eSQ_High];
          }
          if (pr)
            ParseShaderProfile(params, pr);
          break;
        }
      case eVersion:
        break;
      }
    }
  }
  SAFE_DELETE_ARRAY(pScr);
}

const SShaderProfile &CRenderer::GetShaderProfile(EShaderType eST) const
{
  assert((int)eST < sizeof(m_cEF.m_ShaderProfiles)/sizeof(SShaderProfile));

  return m_cEF.m_ShaderProfiles[eST];
}

void CRenderer::SetShaderQuality(EShaderType eST, EShaderQuality eSQ)
{
  eSQ = CLAMP(eSQ, eSQ_Low, eSQ_VeryHigh);
  if (eST == eST_All)
  {
    for (int i=0; i<eST_Max; i++)
    {
      m_cEF.m_ShaderProfiles[i] = m_cEF.m_ShaderFixedProfiles[eSQ];
      m_cEF.m_ShaderProfiles[i].m_iShaderProfileQuality = eSQ;      // good?
    }
  }
  else
  {
    m_cEF.m_ShaderProfiles[eST] = m_cEF.m_ShaderFixedProfiles[eSQ];
    m_cEF.m_ShaderProfiles[eST].m_iShaderProfileQuality = eSQ;      // good?
  }
  if (eST == eST_All || eST == eST_General)
  {
    bool bPS20 = ((m_Features & (RFT_HW_PS2X | RFT_HW_PS30)) == 0) || (eSQ == eSQ_Low);
    CParserBin::SetupForPS20(bPS20);
    m_cEF.m_Bin.InvalidateCache();
    m_cEF.mfReloadAllShaders(FRO_FORCERELOAD, 0);
  }
}

static byte bFirst = 1;

static bool sLoadShader(const char *szName, CShader *& pStorage)
{
  CShader *ef = NULL;
  bool bRes = true;
  if (bFirst)
    iLog->Log("Load System Shader '%s'...", szName);
  ef = gRenDev->m_cEF.mfForName(szName, EF_SYSTEM);
  if (bFirst)
  {
    if (!ef || (ef->m_Flags & EF_NOTFOUND))
    {
      iLog->LogPlus("Fail.\n");
      bRes = false;
    }
    else
      iLog->LogPlus("ok");
  }
  pStorage = ef;
  return bRes;
}

static bool sLoadShaderItem(const char *szName, SShaderItem& pStorage, SInputShaderResources *pRes=NULL)
{
  SShaderItem ef;
  bool bRes = true;
  if (bFirst)
    iLog->Log("Load System Shader '%s'...", szName);
  ef = gRenDev->m_cEF.mfShaderItemForName(szName, true, EF_SYSTEM, pRes);
  if (bFirst)
  {
    if (!ef.m_pShader || (ef.m_pShader->GetFlags() & EF_NOTFOUND))
    {
      iLog->LogPlus("Fail.\n");
      bRes = false;
    }
    else
      iLog->LogPlus("ok");
  }
  pStorage = ef;
  return bRes;
}

void CShaderMan::mfSetDefaults (void)
{
  CShader *ef;

  if (bFirst)
    iLog->Log("Construct Shader '<Default>'...");
  ef = mfNewShader("<Default>");
  m_DefaultShader = ef;
  ef->m_Flags |= EF_SYSTEM;
  if (bFirst)
    iLog->LogPlus("ok");

#ifndef NULL_RENDERER
  //ef = mfForName("testPrepr", EF_BUILD_TREE, NULL);

  //ef = mfForName("Fallback", 0, NULL, 0x8);
  sLoadShader("Fallback", m_ShaderFallback);

  sLoadShader("FixedPipelineEmu", m_ShaderFPEmu);
  //sLoadShader("Cloth", m_ShaderFPEmu);
  sLoadShaderItem("Light.LightStyles", m_ShaderLightStyles);
  sLoadShader("FarTreeSprites", m_ShaderTreeSprites);
  sLoadShader("ShadowBlur", m_ShaderShadowBlur);
  sLoadShader("ShadowMaskGen", m_ShaderShadowMaskGen);
  sLoadShader("AmbientOcclusion", m_ShaderAmbientOcclusion);
	sLoadShader("ScreenSpaceGI", m_ShaderScreenSpaceGI);
  sLoadShader("CloudShadowMaskGen", m_ShaderCloudShadowMaskGen);
  sLoadShader("HDRPostProcess", m_shHDRPostProcess);  
  sLoadShader("PostEffectsGame", m_shPostEffectsGame);  
  sLoadShader("DepthOfField", m_shPostDepthOfField);
  sLoadShader("MotionBlur", m_shPostMotionBlur);
  sLoadShader("Sunshafts", m_shPostSunShafts);
	sLoadShader("Hud3D", m_sh3DHUD);
  sLoadShader("Debug", m_ShaderDebug);
  sLoadShader("LightFlares", m_ShaderLightFlares);
  sLoadShader("Common", m_ShaderCommon);
  sLoadShader("OcclusionTest", m_ShaderOcclTest);     
  sLoadShader("Stereo", m_ShaderStereo);
  //mfForName("Metal", EF_SYSTEM, NULL, 0x1033);



#else
  m_DefaultShaderItem.m_pShader = m_DefaultShader;
  SInputShaderResources ShR;
  m_DefaultShaderItem.m_pShaderResources = new SRenderShaderResources(&ShR);
#endif

//#ifdef DIRECT3D9
  sLoadShader("PostEffects", m_shPostEffects);
  sLoadShader("DeferredShading", m_shDeferredShading);  
	sLoadShader("DeferredCaustics", m_ShaderDeferredCaustics);
  sLoadShader("DeferredRain", m_ShaderDeferredRain);
//#endif

  memset(&gRenDev->m_cEF.m_PF, 0, sizeof(SCGParamsPF) );

  bFirst = 0;

  m_bInitialized = true;
}

bool CShaderMan::mfGatherShadersList(const char *szPath, bool bCheckIncludes, bool bUpdateCRC, std::vector<string> *Names)
{
  struct _finddata_t fileinfo;
  intptr_t handle;
  char nmf[256];
  char dirn[256];

  strcpy_s(dirn, szPath);
  strcat(dirn, "*.*");

  bool bChanged = false;

  handle = gEnv->pCryPak->FindFirst (dirn, &fileinfo);
  if (handle == -1)
    return bChanged;
  do
  {
    if (fileinfo.name[0] == '.')
      continue;
    if (fileinfo.attrib & _A_SUBDIR)
    {
      char ddd[256];
      sprintf(ddd, "%s%s/", szPath, fileinfo.name);

      bChanged = mfGatherShadersList(ddd, bCheckIncludes, bUpdateCRC, Names);
      if (bChanged)
        break;
      continue;
    }
    strcpy_s(nmf, szPath);
    strcat_s(nmf, fileinfo.name);
    int len = strlen(nmf) - 1;
    while (len >= 0 && nmf[len] != '.')
      len--;
    if (len <= 0)
      continue;
    if (bCheckIncludes)
    {
      if (!stricmp(&nmf[len], ".cfi"))
      {
        fpStripExtension(fileinfo.name, nmf);
        SShaderBin *pBin = m_Bin.GetBinShader(nmf, true, 0, &bChanged);
        if (bChanged)
          break;
      }
      continue;
    }
    if (stricmp(&nmf[len], ".cfx"))
      continue;
    fpStripExtension(fileinfo.name, nmf);
    mfAddFXShaderNames(nmf, Names, bUpdateCRC);
  } while (gEnv->pCryPak->FindNext(handle, &fileinfo) != -1);

  gEnv->pCryPak->FindClose (handle);

  return bChanged;
}

void CShaderMan::mfGatherFilesList(const char *szPath, std::vector<CCryName>& Names, int nLevel, bool bUseFilter, bool bMaterial)
{
  struct _finddata_t fileinfo;
  intptr_t handle;
  char nmf[256];
  char dirn[256];

  strcpy_s(dirn, szPath);
  strcat(dirn, "*.*");

  handle = gEnv->pCryPak->FindFirst (dirn, &fileinfo);
  if (handle == -1)
    return;
  do
  {
    if (fileinfo.name[0] == '.')
      continue;
    if (fileinfo.attrib & _A_SUBDIR)
    {
      if (!bUseFilter || nLevel != 1 || (m_ShadersFilter && !stricmp(fileinfo.name, m_ShadersFilter)))
      {
        char ddd[256];
        sprintf(ddd, "%s%s/", szPath, fileinfo.name);

        mfGatherFilesList(ddd, Names, nLevel+1, bUseFilter, bMaterial);
      }
      continue;
    }
    strcpy_s(nmf, szPath);
    strcat_s(nmf, fileinfo.name);
    int len = strlen(nmf) - 1;
    while (len >= 0 && nmf[len] != '.')
      len--;
    if (len <= 0)
      continue;
    if (!bMaterial && stricmp(&nmf[len], ".fxcb"))
      continue;
    if (bMaterial && stricmp(&nmf[len], ".mtl"))
      continue;
    Names.push_back(CCryName(nmf));
  } while (gEnv->pCryPak->FindNext(handle, &fileinfo) != -1);

  gEnv->pCryPak->FindClose (handle);
}

int CShaderMan::mfInitShadersList(std::vector<string> *Names)
{
  // Detect include changes
  bool bChanged = mfGatherShadersList(m_ShadersPath, true, false, Names);

#ifndef NULL_RENDERER
  mfGatherShadersList(m_ShadersPath, false, bChanged, Names);
  return m_ShaderNames.size();
#else
  return 0;
#endif
}

//===================================================================

CShader *CShaderMan::mfNewShader(const char *szName)
{
  CShader *pSH;

  CCryName className = CShader::mfGetClassName();
  CCryName Name = szName;
  CBaseResource *pBR = CBaseResource::GetResource(className, Name, false);
  if (!pBR)
  {
    pSH = new CShader;
    pSH->Register(className, Name);
  }
  else
  {
    pSH = (CShader *)pBR;
    pSH->AddRef();
  }
  if (!m_pContainer)
    m_pContainer = CBaseResource::GetResourcesForClass(className);

  if (pSH->GetID() >= MAX_REND_SHADERS)
  {
    SAFE_RELEASE(pSH);
    iLog->Log("ERROR: MAX_REND_SHADERS hit\n");
    return NULL;
  }

  return pSH;
}


//=========================================================

bool CShaderMan::mfUpdateMergeStatus(SShaderTechnique *hs, std::vector<SCGParam> *p)
{
  if (!p)
    return false;
  for (uint32 n=0; n<p->size(); n++)
  {
    if ((*p)[n].m_Flags & PF_DONTALLOW_DYNMERGE)
    {
      hs->m_Flags |= FHF_NOMERGE;
      break;
    }
  }
  if (hs->m_Flags & FHF_NOMERGE)
    return true;
  return false;
}


//=================================================================================================

static float sFRand()
{
  return cry_rand() / (FLOAT)RAND_MAX;
}

SEnvTexture *SHRenderTarget::GetEnv2D()
{
  CRenderer *rd = gRenDev;
  SEnvTexture *pEnvTex = NULL;
  if (m_nIDInPool >= 0)
  {
    assert(m_nIDInPool < (int)CTexture::s_CustomRT_2D.Num());
    if (m_nIDInPool < (int)CTexture::s_CustomRT_2D.Num())
      pEnvTex = &CTexture::s_CustomRT_2D[m_nIDInPool];
  }
  else
  {
    const CCamera& cam = rd->GetCamera();
    Matrix33 orientation = Matrix33(cam.GetMatrix());
    Ang3 Angs = CCamera::CreateAnglesYPR(orientation);
    Vec3 Pos = cam.GetPosition();
    bool bReflect = false;
    if (m_nFlags & (FRT_CAMERA_REFLECTED_PLANE | FRT_CAMERA_REFLECTED_WATERPLANE))
      bReflect = true;
    pEnvTex = CTexture::FindSuitableEnvTex(Pos, Angs, true, 0, false, rd->m_RP.m_pShader, rd->m_RP.m_pShaderResources, rd->m_RP.m_pCurObject, bReflect, rd->m_RP.m_pRE, NULL);
  }
  return pEnvTex;
}

SEnvTexture *SHRenderTarget::GetEnvCM()
{
  CRenderer *rd = gRenDev;
  SEnvTexture *pEnvTex = NULL;

  if (m_nIDInPool >= 0)
  {
    assert(m_nIDInPool < (int)CTexture::s_CustomRT_CM.Num());
    pEnvTex = &CTexture::s_CustomRT_CM[m_nIDInPool];
  }
  else
  {
    Vec3 vPos = rd->m_RP.m_pCurObject->GetTranslation();
    pEnvTex = CTexture::FindSuitableEnvCMap(vPos, true, 0, 0);
  }
  return pEnvTex;
}

void SHRenderTarget::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->Add( *this );
	pSizer->AddObject( m_TargetName );
	pSizer->AddObject(  m_pTarget[0] );
	pSizer->AddObject(  m_pTarget[1] );	
}

// Update TexGen and TexTransform matrices for current material texture
void SEfResTexture::Update(int nTSlot)
{
  int i;
  CRenderer *rd = gRenDev;

  assert( nTSlot < MAX_TMU );
  rd->m_RP.m_ShaderTexResources[nTSlot] = this;

  if (m_TexModificator.m_bDontUpdate)
  {
    if (m_Sampler.m_pTex)
    {
      ETEX_Type eTT = m_Sampler.m_pTex->GetTexType();
      if (eTT == eTT_Cube || eTT == eTT_AutoCube)
        m_TexModificator.m_UpdateFlags |= HWMD_TCTYPE0;
      else
        m_TexModificator.m_UpdateFlags &= ~HWMD_TCTYPE0;
      m_TexModificator.m_UpdateFlags &= ~(HWMD_TCPROJ0 | HWMD_TCGOL0);
      eTT = (ETEX_Type)m_Sampler.m_eTexType;
      if (eTT == eTT_Auto2D) 
      {
        SEnvTexture *pEnv = m_Sampler.m_pTarget->GetEnv2D();
        assert(pEnv && pEnv->m_pTex && pEnv->m_pTex->m_pTexture);
        if (pEnv && pEnv->m_pTex)
        {
          m_TexModificator.m_UpdateFlags |= HWMD_TCGOL0;
          m_TexModificator.m_TexGenMatrix.Multiply(GetTransposed44(Matrix44A(rd->m_RP.m_pCurObject->m_II.m_Matrix)), pEnv->m_Matrix);
          m_TexModificator.m_TexGenMatrix.Transpose(m_TexModificator.m_TexGenMatrix);
          m_TexModificator.m_UpdateFlags |= HWMD_TCPROJ0;
        }
      }
    }
    else
    {
      if (m_Sampler.m_pDynTexSource)
      {
        m_TexModificator.m_TexMatrix.SetIdentity();
        m_Sampler.m_pDynTexSource->GetTexGenInfo(m_TexModificator.m_TexMatrix.m30, m_TexModificator.m_TexMatrix.m31, 
          m_TexModificator.m_TexMatrix.m00, m_TexModificator.m_TexMatrix.m11);
        m_TexModificator.m_UpdateFlags |= HWMD_TCM0;
      }
    }

    if (m_TexModificator.m_bTexGenProjected)
      m_TexModificator.m_UpdateFlags |= HWMD_TCPROJ0;
    if (nTSlot < 4)
      rd->m_RP.m_FlagsShader_MD |= m_TexModificator.m_UpdateFlags<<nTSlot;
    return;
  }

  int nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameID;
  if (m_TexModificator.m_nFrameUpdated == nFrameID && m_TexModificator.m_nLastRecursionLevel == SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID])
  {
    if (nTSlot < 4)
      rd->m_RP.m_FlagsShader_MD |= m_TexModificator.m_UpdateFlags<<nTSlot;
    return;
  }
  m_TexModificator.m_nFrameUpdated = nFrameID;
  m_TexModificator.m_nLastRecursionLevel = SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID];
  m_TexModificator.m_UpdateFlags = 0;

  bool bTr = false;
  bool bTranspose = false;
  Plane Pl;
  Plane PlTr;
  if (m_TexModificator.m_Tiling[0] == 0)
    m_TexModificator.m_Tiling[0] = 1.0f;
  if (m_TexModificator.m_Tiling[1] == 0)
    m_TexModificator.m_Tiling[1] = 1.0f;

  if (m_TexModificator.m_eUMoveType != ETMM_NoChange || m_TexModificator.m_eVMoveType != ETMM_NoChange || m_TexModificator.m_eRotType != ETMR_NoChange 
   || m_TexModificator.m_Offs[0]!=0.0f || m_TexModificator.m_Offs[1]!=0.0f || m_TexModificator.m_Tiling[0]!=1.0f || m_TexModificator.m_Tiling[1]!=1.0f)
  {
    m_TexModificator.m_TexMatrix.SetIdentity();
    float fTime = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;

    bTr = true;

    switch(m_TexModificator.m_eRotType)
    {
      case ETMR_Fixed:
        {
          m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix *
            Matrix44(1,0,0,0,
            0,1,0,0,
            m_TexModificator.m_RotOscCenter[0],m_TexModificator.m_RotOscCenter[1],1,0,
            0,0,0,1);
          if (m_TexModificator.m_RotOscPhase[0])
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationX(Word2Degr(m_TexModificator.m_RotOscPhase[0])*PI/180.0f);
          if (m_TexModificator.m_RotOscPhase[1])
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationY(Word2Degr(m_TexModificator.m_RotOscPhase[1])*PI/180.0f);
          if (m_TexModificator.m_RotOscPhase[2])
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationZ(Word2Degr(m_TexModificator.m_RotOscPhase[2])*PI/180.0f);
          m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix *
            Matrix44(1,0,0,0,
            0,1,0,0,
            -m_TexModificator.m_RotOscCenter[0],-m_TexModificator.m_RotOscCenter[1],1,0,
            0,0,0,1);
        }
        break;

      case ETMR_Constant:
        {
          fTime *= 1000.0f;
          float fxAmp = Word2Degr(m_TexModificator.m_RotOscAmplitude[0]) * fTime * PI/180.0f;
          float fyAmp = Word2Degr(m_TexModificator.m_RotOscAmplitude[1]) * fTime * PI/180.0f;
          float fzAmp = Word2Degr(m_TexModificator.m_RotOscAmplitude[2]) * fTime * PI/180.0f;

          //translation matrix for rotation center
          m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix *
            Matrix44(	1,0,0,0,
                      0,1,0,0,
                      0,0,1,0,
                      -m_TexModificator.m_RotOscCenter[0],-m_TexModificator.m_RotOscCenter[1],-m_TexModificator.m_RotOscCenter[2],1);

          //rotation matrix
          if (fxAmp)
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * GetTransposed44(Matrix44A(Matrix33::CreateRotationX(fxAmp)));
          if (fyAmp)
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * GetTransposed44(Matrix44A(Matrix33::CreateRotationY(fyAmp)));
          if (fzAmp)
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix* GetTransposed44(Matrix44A(Matrix33::CreateRotationZ(fzAmp)));

          //translation matrix for rotation center
          m_TexModificator.m_TexMatrix =  m_TexModificator.m_TexMatrix *
            Matrix44(	1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            m_TexModificator.m_RotOscCenter[0],m_TexModificator.m_RotOscCenter[1],m_TexModificator.m_RotOscCenter[2],1);
                  
        }
        break;

      case ETMR_Oscillated:
        {
          m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix *
            Matrix44(1,0,0,0,
            0,1,0,0,
            -m_TexModificator.m_RotOscCenter[0],-m_TexModificator.m_RotOscCenter[1],1,0,
            0,0,0,1);
          float S_X = fTime * Word2Degr(m_TexModificator.m_RotOscRate[0]);
          float d_X = Word2Degr(m_TexModificator.m_RotOscAmplitude[0]) * cry_sinf(2.0f * PI * ((S_X - cry_floorf(S_X)) + Word2Degr(m_TexModificator.m_RotOscPhase[0])));
          float S_Y = fTime * Word2Degr(m_TexModificator.m_RotOscRate[1]);
          float d_Y = Word2Degr(m_TexModificator.m_RotOscAmplitude[1]) * cry_sinf(2.0f * PI * ((S_Y - cry_floorf(S_Y)) + Word2Degr(m_TexModificator.m_RotOscPhase[1])));
          float S_Z = fTime * Word2Degr(m_TexModificator.m_RotOscRate[2]);
          float d_Z = Word2Degr(m_TexModificator.m_RotOscAmplitude[2]) * cry_sinf(2.0f * PI * ((S_Z - cry_floorf(S_Z)) + Word2Degr(m_TexModificator.m_RotOscPhase[2])));
          if (d_X)
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationX(d_X);
          if (d_Y)
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationY(d_Y);
          if (d_Z)
            m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationZ(d_Z);
          m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix *
            Matrix44(1,0,0,0,
            0,1,0,0,
            -m_TexModificator.m_RotOscCenter[0],-m_TexModificator.m_RotOscCenter[1],1,0,
            0,0,0,1);
        }
        break;
    }


    float Su = rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * m_TexModificator.m_UOscRate;
    float Sv = rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * m_TexModificator.m_VOscRate;
    switch(m_TexModificator.m_eUMoveType)
    {
      case ETMM_Pan:
        {
          float  du = m_TexModificator.m_UOscAmplitude * cry_sinf(2.0f * PI * (Su - cry_floorf(Su)) + 2.f * PI * m_TexModificator.m_UOscPhase);
          m_TexModificator.m_TexMatrix(3,0) = du;
        }
        break;
      case ETMM_Fixed:
        {
          float du = m_TexModificator.m_UOscRate;
          m_TexModificator.m_TexMatrix(3,0) = du;
        }
        break;
      case ETMM_Constant:
        {
          float du = m_TexModificator.m_UOscAmplitude * Su; //(Su - cry_floorf(Su));
          m_TexModificator.m_TexMatrix(3,0) = du;
        }
        break;
      case ETMM_Jitter:
        {
          if( m_TexModificator.m_LastUTime < 1.0f || m_TexModificator.m_LastUTime > Su + 1.0f )
            m_TexModificator.m_LastUTime = m_TexModificator.m_UOscPhase + cry_floorf(Su);
          if( Su-m_TexModificator.m_LastUTime > 1.0f )
          {
            m_TexModificator.m_CurrentUJitter = sFRand() * m_TexModificator.m_UOscAmplitude;
            m_TexModificator.m_LastUTime = m_TexModificator.m_UOscPhase + cry_floorf(Su);
          }
          m_TexModificator.m_TexMatrix(3,0) = m_TexModificator.m_CurrentUJitter;
        }
        break;
      case ETMM_Stretch:
        {
          float du = m_TexModificator.m_UOscAmplitude * cry_sinf(2.0f * PI * (Su - cry_floorf(Su)) + 2.0f * PI * m_TexModificator.m_UOscPhase);
          m_TexModificator.m_TexMatrix(0,0) = 1.0f+du;
        }
        break;
      case ETMM_StretchRepeat:
        {
          float du = m_TexModificator.m_UOscAmplitude * cry_sinf(0.5f * PI * (Su - cry_floorf(Su)) + 2.0f * PI * m_TexModificator.m_UOscPhase);
          m_TexModificator.m_TexMatrix(0,0) = 1.0f+du;
        }
        break;
    }

    switch(m_TexModificator.m_eVMoveType)
    {
      case ETMM_Pan:
        {
          float dv = m_TexModificator.m_VOscAmplitude * cry_sinf(2.0f * PI * (Sv - cry_floorf(Sv)) + 2.0f * PI * m_TexModificator.m_VOscPhase);
          m_TexModificator.m_TexMatrix(3,1) = dv;
        }
        break;
      case ETMM_Fixed:
        {
          float dv = m_TexModificator.m_VOscRate;
          m_TexModificator.m_TexMatrix(3,1) = dv;
        }
        break;
      case ETMM_Constant:
        {
          float dv = m_TexModificator.m_VOscAmplitude * Sv; //(Sv - cry_floorf(Sv));
          m_TexModificator.m_TexMatrix(3,1) = dv;
        }
        break;
      case ETMM_Jitter:
        {
          if( m_TexModificator.m_LastVTime < 1.0f || m_TexModificator.m_LastVTime > Sv + 1.0f )
            m_TexModificator.m_LastVTime = m_TexModificator.m_VOscPhase + cry_floorf(Sv);
          if( Sv-m_TexModificator.m_LastVTime > 1.0f )
          {
            m_TexModificator.m_CurrentVJitter = sFRand() * m_TexModificator.m_VOscAmplitude;
            m_TexModificator.m_LastVTime = m_TexModificator.m_VOscPhase + cry_floorf(Sv);
          }
          m_TexModificator.m_TexMatrix(3,1) = m_TexModificator.m_CurrentVJitter;
        }
        break;
      case ETMM_Stretch:
        {
          float dv = m_TexModificator.m_VOscAmplitude * cry_sinf(2.0f * PI * (Sv - cry_floorf(Sv)) + 2.0f * PI * m_TexModificator.m_VOscPhase);
          m_TexModificator.m_TexMatrix(1,1) = 1.0f+dv;
        }
        break;
      case ETMM_StretchRepeat:
        {
          float dv = m_TexModificator.m_VOscAmplitude * cry_sinf(0.5f * PI * (Sv - cry_floorf(Sv)) + 2.0f * PI * m_TexModificator.m_VOscPhase);
          m_TexModificator.m_TexMatrix(1,1) = 1.0f+dv;
        }
        break;
    }

    if (m_TexModificator.m_Offs[0]!=0.0f || m_TexModificator.m_Offs[1]!=0.0f || m_TexModificator.m_Tiling[0]!=1.0f || m_TexModificator.m_Tiling[1]!=1.0f || m_TexModificator.m_Rot[0] || m_TexModificator.m_Rot[1] || m_TexModificator.m_Rot[2])
    {
      float du = m_TexModificator.m_Offs[0];
      float dv = m_TexModificator.m_Offs[1];
      float su = m_TexModificator.m_Tiling[0];
      float sv = m_TexModificator.m_Tiling[1];
      m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix *
        Matrix44(su,0,0,0,
                 0,sv,0,0,
                 0,0,1,0,
                 du,dv,0,1);
      if (m_TexModificator.m_Rot[0])
        m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationX(Word2Degr(m_TexModificator.m_Rot[0])*PI/180.0f);
      if (m_TexModificator.m_Rot[1])
        m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationY(Word2Degr(m_TexModificator.m_Rot[1])*PI/180.0f);
      if (m_TexModificator.m_Rot[2])
        m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix * Matrix33::CreateRotationZ(Word2Degr(m_TexModificator.m_Rot[2])*PI/180.0f);
      if (m_TexModificator.m_Rot[0] || m_TexModificator.m_Rot[1] || m_TexModificator.m_Rot[2])
        m_TexModificator.m_TexMatrix = m_TexModificator.m_TexMatrix *
        Matrix44(su,0,0,0,
        0,sv,0,0,
        0,0,1,0,
        -du,-dv,0,1);
    }
  }

  if (m_TexModificator.m_eTGType != ETG_Stream)
  {
    switch (m_TexModificator.m_eTGType)
    {
      case ETG_World:
        {
          m_TexModificator.m_UpdateFlags |= HWMD_TCGOL0;
          for (i=0; i<4; i++)
          {
            memset(&Pl, 0, sizeof(Pl));
            float *fPl = (float *)&Pl;
            fPl[i] = 1.0f;
            PlTr = TransformPlane2_NoTrans(GetTransposed44(Matrix44A(rd->m_RP.m_pCurObject->m_II.m_Matrix)), Pl);
            m_TexModificator.m_TexGenMatrix(i,0) = PlTr.n.x;
            m_TexModificator.m_TexGenMatrix(i,1) = PlTr.n.y;
            m_TexModificator.m_TexGenMatrix(i,2) = PlTr.n.z;
            m_TexModificator.m_TexGenMatrix(i,3) = PlTr.d;
          }
        }
        break;
      case ETG_Camera:
        {
          m_TexModificator.m_UpdateFlags |= HWMD_TCGOL0;
          for (i=0; i<4; i++)
          {
            memset(&Pl, 0, sizeof(Pl));
            float *fPl = (float *)&Pl;
            fPl[i] = 1.0f;
            PlTr = TransformPlane2_NoTrans(rd->m_ViewMatrix, Pl);
            m_TexModificator.m_TexGenMatrix(i,0) = PlTr.n.x;
            m_TexModificator.m_TexGenMatrix(i,1) = PlTr.n.y;
            m_TexModificator.m_TexGenMatrix(i,2) = PlTr.n.z;
            m_TexModificator.m_TexGenMatrix(i,3) = PlTr.d;
          }
        }
        break;
      case ETG_WorldEnvMap:
        {
          m_TexModificator.m_UpdateFlags |= HWMD_TCGRM0;
          if (bTr)
            m_TexModificator.m_TexMatrix = rd->m_CameraMatrix * m_TexModificator.m_TexMatrix;
          else
            m_TexModificator.m_TexMatrix = rd->m_CameraMatrix;
          bTr = true;
          if (m_Sampler.m_eTexType == eTT_Cube)
            bTranspose = true;
        }
        break;
      case ETG_CameraEnvMap:
        {
          m_TexModificator.m_UpdateFlags |= HWMD_TCGRM0;
        }
        break;
      case ETG_SphereMap:
        {
          m_TexModificator.m_UpdateFlags |= HWMD_TCGSM0;
          if (bTr)
            m_TexModificator.m_TexMatrix = GetTransposed44(Matrix44A(rd->m_RP.m_pCurObject->m_II.m_Matrix)) * m_TexModificator.m_TexMatrix;
          else
            m_TexModificator.m_TexMatrix = GetTransposed44(Matrix44A(rd->m_RP.m_pCurObject->m_II.m_Matrix));
          //bTr = true;
          //bTranspose = true;
        }
        break;
      case ETG_NormalMap:
        {
          m_TexModificator.m_UpdateFlags |= HWMD_TCGNM0;
        }
        break;
    }
  }

  if (bTr)
  {
    m_TexModificator.m_UpdateFlags |= HWMD_TCM0;
    if (m_TexModificator.m_bTexGenProjected)
    {
      m_TexModificator.m_TexMatrix(0,3) = m_TexModificator.m_TexMatrix(0,2);
      m_TexModificator.m_TexMatrix(1,3) = m_TexModificator.m_TexMatrix(1,2);
      m_TexModificator.m_TexMatrix(2,3) = m_TexModificator.m_TexMatrix(2,2);
      m_TexModificator.m_TexMatrix(3,3) = m_TexModificator.m_TexMatrix(3,2);
    }
    else
    {
      m_TexModificator.m_TexMatrix(0,3) = m_TexModificator.m_TexMatrix(0,2);
      m_TexModificator.m_TexMatrix(1,3) = m_TexModificator.m_TexMatrix(1,2);
      m_TexModificator.m_TexMatrix(2,3) = m_TexModificator.m_TexMatrix(2,2);
    }
  }
  if (!(m_TexModificator.m_UpdateFlags & (HWMD_TCG | HWMD_TCM)))
    m_TexModificator.m_bDontUpdate = true;
  if (m_Sampler.m_pTex)
  {
    ETEX_Type eTT = m_Sampler.m_pTex->GetTexType();
    if (eTT == eTT_Cube || eTT == eTT_AutoCube)
      m_TexModificator.m_UpdateFlags |= HWMD_TCTYPE0;
    else
      m_TexModificator.m_UpdateFlags &= ~HWMD_TCTYPE0;
  }
  if (m_TexModificator.m_bTexGenProjected)
    m_TexModificator.m_UpdateFlags |= HWMD_TCPROJ0;

  if (nTSlot < 4)
    rd->m_RP.m_FlagsShader_MD |= m_TexModificator.m_UpdateFlags<<nTSlot;
}

//---------------------------------------------------------------------------
// Wave evaluator

float CShaderMan::EvalWaveForm(SWaveForm *wf) 
{
  int val;

  float Amp;
  float Freq;
  float Phase;
  float Level;

  if (wf->m_Flags & WFF_LERP)
  {
    val = (int)(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime * 597.0f);
    val &= SRenderPipeline::sSinTableCount-1;
    float fLerp = gRenDev->m_RP.m_tSinTable[val] * 0.5f + 0.5f;

    if (wf->m_Amp != wf->m_Amp1)
      Amp = LERP(wf->m_Amp, wf->m_Amp1, fLerp);
    else
      Amp = wf->m_Amp;

    if (wf->m_Freq != wf->m_Freq1)
      Freq = LERP(wf->m_Freq, wf->m_Freq1, fLerp);
    else
      Freq = wf->m_Freq;

    if (wf->m_Phase != wf->m_Phase1)
      Phase = LERP(wf->m_Phase, wf->m_Phase1, fLerp);
    else
      Phase = wf->m_Phase;

    if (wf->m_Level != wf->m_Level1)
      Level = LERP(wf->m_Level, wf->m_Level1, fLerp);
    else
      Level = wf->m_Level;
  }
  else
  {
    Level = wf->m_Level;
    Amp = wf->m_Amp;
    Phase = wf->m_Phase;
    Freq = wf->m_Freq;
  }

  switch(wf->m_eWFType)
  {
  case eWF_None:
    Warning( "WARNING: CShaderMan::EvalWaveForm called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
    break;

  case eWF_Sin:
    val = (int)((gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime*Freq+Phase)*(float)SRenderPipeline::sSinTableCount);
    return Amp*gRenDev->m_RP.m_tSinTable[val&(SRenderPipeline::sSinTableCount-1)]+Level;

    // Other wave types aren't supported anymore
  case eWF_HalfSin:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_InvHalfSin:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_SawTooth:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_InvSawTooth:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_Square:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_Triangle:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_Hill:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_InvHill:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  default:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    break;
  }
  return 1;
}

float CShaderMan::EvalWaveForm(SWaveForm2 *wf) 
{
  int val;

  switch(wf->m_eWFType)
  {
  case eWF_None:
    //Warning( 0,0,"WARNING: CShaderMan::EvalWaveForm called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
    break;

  case eWF_Sin:
    val = (int)((gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime*wf->m_Freq+wf->m_Phase)*(float)SRenderPipeline::sSinTableCount);
    return wf->m_Amp*gRenDev->m_RP.m_tSinTable[val&(SRenderPipeline::sSinTableCount-1)]+wf->m_Level;

    // Other wave types aren't supported anymore
  case eWF_HalfSin:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_InvHalfSin:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_SawTooth:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_InvSawTooth:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_Square:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_Triangle:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_Hill:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  case eWF_InvHill:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    assert(0);
    return 0;

  default:
    Warning( "WARNING: CShaderMan::EvalWaveForm: bad WaveType '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
    break;
  }
  return 1;
}

float CShaderMan::EvalWaveForm2(SWaveForm *wf, float frac) 
{
  int val;

  if (!(wf->m_Flags & WFF_CLAMP))
    switch(wf->m_eWFType)
  {
    case eWF_None:
      Warning( "CShaderMan::EvalWaveForm2 called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
      break;

    case eWF_Sin:
      val = QRound((frac*wf->m_Freq+wf->m_Phase)*(float)SRenderPipeline::sSinTableCount);
      val &= (SRenderPipeline::sSinTableCount-1);
      return wf->m_Amp*gRenDev->m_RP.m_tSinTable[val]+wf->m_Level;

      // Other wave types aren't supported anymore
    case eWF_SawTooth:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_InvSawTooth:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_Square:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_Triangle:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_Hill:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_InvHill:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    default:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      break;
  }
  else
  switch(wf->m_eWFType)
  {
    case eWF_None:
      Warning( "Warning: CShaderMan::EvalWaveForm2 called with 'EWF_None' in Shader '%s'\n", gRenDev->m_RP.m_pShader->GetName());
      break;

    case eWF_Sin:
      val = QRound((frac*wf->m_Freq+wf->m_Phase)*(float)SRenderPipeline::sSinTableCount);
      val &= (SRenderPipeline::sSinTableCount-1);
      return wf->m_Amp*gRenDev->m_RP.m_tSinTable[val]+wf->m_Level;

      // Other wave types aren't supported anymore
    case eWF_SawTooth:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_InvSawTooth:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_Square:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_Triangle:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_Hill:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    case eWF_InvHill:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      assert(0);
      return 0;

    default:
      Warning( "Warning: CShaderMan::EvalWaveForm2: bad EWF '%d' in Shader '%s'\n", wf->m_eWFType, gRenDev->m_RP.m_pShader->GetName());
      break;
  }
  return 1;
}

void CShaderMan::mfBeginFrame()
{
  CHWShader::mfBeginFrame(2);
  if (CRenderer::CV_r_shaderslazyunload)
    CHWShader::mfLazyUnload();
}

EHWShaderClass CHWShader::mfStringProfile(const char *profile)
{
  EHWShaderClass Profile = eHWSC_Max;
  if (!strncmp(profile, "vs_4_0", 6) || !strncmp(profile, "vs_3_0", 6))
    Profile = eHWSC_Vertex;
  else
  if (!strncmp(profile, "ps_4_0", 6) || !strncmp(profile, "ps_3_0", 6))
    Profile = eHWSC_Pixel;
  else
  if (!strncmp(profile, "gs_4_0", 6))
    Profile = eHWSC_Geometry;
  else
    assert(0);

  return Profile;
}
EHWShaderClass CHWShader::mfStringClass(const char *szClass)
{
  EHWShaderClass Profile = eHWSC_Max;
  if (!strnicmp(szClass, "VS", 2))
    Profile = eHWSC_Vertex;
  else
  if (!strnicmp(szClass, "PS", 2))
    Profile = eHWSC_Pixel;
  else
  if (!strnicmp(szClass, "GS", 2))
    Profile = eHWSC_Geometry;
  else
    assert(0);

  return Profile;
}
const char *CHWShader::mfProfileString(EHWShaderClass eClass)
{
  char *szProfile = "Unknown";
  switch (eClass)
  {
    case eHWSC_Vertex:
      if (CParserBin::m_bD3D11)
        szProfile = "vs_4_0";
      else
      if (CParserBin::m_bPS3 || CParserBin::m_bXenon)
        szProfile = "vs_3_0";
      else
        szProfile = "vs_3_0";
      break;
    case eHWSC_Pixel:
      if (CParserBin::m_bD3D11)
        szProfile = "ps_4_0";
      else
      if (CParserBin::m_bPS3 || CParserBin::m_bXenon)
        szProfile = "ps_3_0";
      else
        szProfile = "ps_3_0";
      break;
    case eHWSC_Geometry:
      szProfile = "gs_4_0";
      break;
    default:
      assert(0);
  }
  return szProfile;
}
const char *CHWShader::mfClassString(EHWShaderClass eClass)
{
  char *szClass = "Unknown";
  switch (eClass)
  {
    case eHWSC_Vertex:
      szClass = "VS";
      break;
    case eHWSC_Pixel:
      szClass = "PS";
      break;
    case eHWSC_Geometry:
      szClass = "GS";
      break;
    default:
      assert(0);
  }
  return szClass;
}

//==========================================================================================================

inline bool sCompareRes(SRenderShaderResources* a, SRenderShaderResources* b)
{
  if (!a || !b)
    return a < b;

  if (a->m_AlphaRef != b->m_AlphaRef)
    return a->m_AlphaRef < b->m_AlphaRef;

  if (a->Opacity() != b->Opacity())
    return a->Opacity() < b->Opacity();

  if (a->m_pDeformInfo != b->m_pDeformInfo)
    return a->m_pDeformInfo < b->m_pDeformInfo;

  if (a->m_pDetailDecalInfo != b->m_pDetailDecalInfo)
    return a->m_pDetailDecalInfo < b->m_pDetailDecalInfo;

  if (a->m_RTargets.Num() != b->m_RTargets.Num())
    return a->m_RTargets.Num() < b->m_RTargets.Num();

  if ((a->m_ResFlags&MTL_FLAG_2SIDED) != (b->m_ResFlags&MTL_FLAG_2SIDED))
    return (a->m_ResFlags&MTL_FLAG_2SIDED) < (b->m_ResFlags&MTL_FLAG_2SIDED);

  ITexture *pTA = NULL;
  ITexture *pTB = NULL;
  if (a->m_Textures[EFTT_GLOSS])
    pTA = a->m_Textures[EFTT_GLOSS]->m_Sampler.m_pITex;
  if (b->m_Textures[EFTT_GLOSS])
    pTB = b->m_Textures[EFTT_GLOSS]->m_Sampler.m_pITex;
  if (pTA != pTB)
    return pTA < pTB;

  pTA = NULL;
  pTB = NULL;
  if (a->m_Textures[EFTT_DIFFUSE])
    pTA = a->m_Textures[EFTT_DIFFUSE]->m_Sampler.m_pITex;
  if (b->m_Textures[EFTT_DIFFUSE])
    pTB = b->m_Textures[EFTT_DIFFUSE]->m_Sampler.m_pITex;
  if (pTA != pTB)
    return pTA < pTB;

  pTA = NULL;
  pTB = NULL;
  if (a->m_Textures[EFTT_BUMP])
    pTA = a->m_Textures[EFTT_BUMP]->m_Sampler.m_pITex;
  if (b->m_Textures[EFTT_BUMP])
    pTB = b->m_Textures[EFTT_BUMP]->m_Sampler.m_pITex;
  if (pTA != pTB)
    return pTA < pTB;

  pTA = NULL;
  pTB = NULL;
  if (a->m_Textures[EFTT_ENV])
    pTA = a->m_Textures[EFTT_ENV]->m_Sampler.m_pITex;
  if (b->m_Textures[EFTT_ENV])
    pTB = b->m_Textures[EFTT_ENV]->m_Sampler.m_pITex;
  if (pTA != pTB)
    return pTA < pTB;

  pTA = NULL;
  pTB = NULL;
  if (a->m_Textures[EFTT_DETAIL_OVERLAY])
    pTA = a->m_Textures[EFTT_DETAIL_OVERLAY]->m_Sampler.m_pITex;
  if (b->m_Textures[EFTT_DETAIL_OVERLAY])
    pTB = b->m_Textures[EFTT_DETAIL_OVERLAY]->m_Sampler.m_pITex;  
  return (pTA < pTB);
}

inline bool sCompareShd(CBaseResource *a, CBaseResource* b)
{
  if (!a || !b)
    return a < b;

  CShader *pA = (CShader *)a;
  CShader *pB = (CShader *)b;
  SShaderPass *pPA = NULL;
  SShaderPass *pPB = NULL;
  if (pA->m_HWTechniques.Num() && pA->m_HWTechniques[0]->m_Passes.Num())
    pPA = &pA->m_HWTechniques[0]->m_Passes[0];
  if (pB->m_HWTechniques.Num() && pB->m_HWTechniques[0]->m_Passes.Num())
    pPB = &pB->m_HWTechniques[0]->m_Passes[0];
  if (!pPA || !pPB)
    return pPA < pPB;

  if (pPA->m_VShader != pPB->m_VShader)
    return pPA->m_VShader < pPB->m_VShader;
  return (pPA->m_PShader < pPB->m_PShader);
}

void CShaderMan::mfSortResources()
{
  uint32 i;
  for (i=0; i<MAX_TMU; i++)
  {
    gRenDev->m_RP.m_ShaderTexResources[i] = NULL;
  }
  iLog->Log("-- Presort shaders by states...");
  //return;
  std::sort(&CShader::m_ShaderResources_known.begin()[1], CShader::m_ShaderResources_known.end(), sCompareRes);

  for (i=1; i<CShader::m_ShaderResources_known.Num(); i++)
  {
    if (CShader::m_ShaderResources_known[i])
      CShader::m_ShaderResources_known[i]->m_Id = i;
  }

  SResourceContainer *pRL = CShaderMan::m_pContainer;
  assert(pRL);
  if (pRL)
  {
    std::sort(pRL->m_RList.begin(), pRL->m_RList.end(), sCompareShd);
    for (i=0; i<pRL->m_RList.size(); i++)
    {
      CShader *pSH = (CShader *)pRL->m_RList[i];
      if (pSH)
        pSH->SetID(i);
    }

    pRL->m_AvailableIDs.clear();
    for (i=0; i<pRL->m_RList.size(); i++)
    {
      CShader *pSH = (CShader *)pRL->m_RList[i];
      if (!pSH)
        pRL->m_AvailableIDs.push_back(i);
    }
  }
}

#include UNIQUE_VIRTUAL_WRAPPER(IShader)
#include UNIQUE_VIRTUAL_WRAPPER(IRenderShaderResources)
