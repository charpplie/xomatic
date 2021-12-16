/*=============================================================================
D3DHWShader.cpp : D3D specific shaders support.
Copyright (c) 2001-2005 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "IDirectBee.h"
#include <crc32.h>
#include "../Common/Shaders/RemoteCompiler.h"

#ifdef WIN64
	#pragma warning(disable: 4244)
#endif

#if defined(OPENGL)
SGlobalConstMap CHWShader_D3D::sGlobalConsts[CHWShader_D3D::scSGlobalConstMapCount];
#endif
//=======================================================================


DEFINE_ALIGNED_DATA(Vec4, CHWShader_D3D::m_CurPSParams[MAX_CONSTANTS_PS], 16);
DEFINE_ALIGNED_DATA(Vec4, CHWShader_D3D::m_CurVSParams[MAX_CONSTANTS_VS], 16);
#ifndef XENON
#if defined (DIRECT3D9) || defined(OPENGL)
DEFINE_ALIGNED_DATA(Vec4, CHWShader_D3D::m_CurPSParamsI[16], 16);
DEFINE_ALIGNED_DATA(Vec4, CHWShader_D3D::m_CurVSParamsI[16], 16);
#elif defined (DIRECT3D10)
ID3D11Buffer **CHWShader_D3D::m_pCB[eHWSC_Max][CB_MAX];
ID3D11Buffer *CHWShader_D3D::m_pCurReqCB[eHWSC_Max][CB_MAX];
void *CHWShader_D3D::m_pCurDevCB[eHWSC_Max][CB_MAX];
Vec4 *CHWShader_D3D::m_pDataCB[eHWSC_Max][CB_MAX];
int CHWShader_D3D::m_nCurMaxVecs[eHWSC_Max][CB_MAX];
int CHWShader_D3D::m_nMax_PF_Vecs[eHWSC_Max];
int CHWShader_D3D::m_nMax_SG_Vecs[eHWSC_Max];
ID3D11Buffer *CHWShader_D3D::m_pLightCB[eHWSC_Max];
CHWShader_D3D::SHWSInstance *CHWShader_D3D::m_pCurInstVS;
CHWShader_D3D::SHWSInstance *CHWShader_D3D::m_pCurInstPS;
CHWShader_D3D::SHWSInstance *CHWShader_D3D::m_pCurInstGS;
#endif



#endif

std::vector<SCGParam> CHWShader_D3D::m_PF_Params[eHWSC_Max];
std::vector<SCGParam> CHWShader_D3D::m_SG_Params[eHWSC_Max];
std::vector<SCGParam> CHWShader_D3D::m_CM_Params[eHWSC_Max];

CHWShader_D3D::InstanceMap CHWShader_D3D::m_SharedInsts;

DynArray<SShaderTechniqueStat> g_SelectedTechs;

int CHWShader_D3D::m_FrameObj;

bool CHWShader_D3D::ms_bInitShaders = true;

int CHWShader_D3D::m_PSParamsToCommit[256];
int CHWShader_D3D::m_NumPSParamsToCommit;
int CHWShader_D3D::m_VSParamsToCommit[256];
int CHWShader_D3D::m_NumVSParamsToCommit;

int CHWShader_D3D::m_nResetDeviceFrame = -1;
int CHWShader_D3D::m_nInstFrame = -1;

SD3DShader *CHWShader::m_pCurPS;
SD3DShader *CHWShader::m_pCurVS;
SD3DShader *CHWShader::m_pCurGS;

FXShaderCache CHWShader::m_ShaderCache;
FXShaderCacheNames CHWShader::m_ShaderCacheList;

//extern float fTime0;
//extern float fTime1;
//extern float fTime2;

int SD3DShader::Release(EHWShaderClass eSHClass, int nSize)
{
  m_nRef--;
  if (m_nRef)
		return m_nRef;
  void *pHandle = m_pHandle;
  delete this;
	if (!pHandle)
		return 0;
  if (eSHClass == eHWSC_Pixel)
    CHWShader_D3D::m_nDevicePSDataSize -= nSize;
  else
    CHWShader_D3D::m_nDeviceVSDataSize -= nSize;

#if defined (DIRECT3D9) || defined (OPENGL)
  if (eSHClass == eHWSC_Pixel)
    return ((IDirect3DPixelShader9*)pHandle)->Release();
  else
    return ((IDirect3DVertexShader9*)pHandle)->Release();
#elif defined (DIRECT3D10)
  if (eSHClass == eHWSC_Pixel)
    return ((ID3D11PixelShader*)pHandle)->Release();
  else
  if (eSHClass == eHWSC_Vertex)
    return ((ID3D11VertexShader*)pHandle)->Release();
  else
  if (GEOMETRYSHADER_SUPPORT && eSHClass == eHWSC_Geometry)
    return ((ID3D11GeometryShader*)pHandle)->Release();
  else
  {
    assert(0);
    return 0;
  }
#endif
}

void CHWShader_D3D::SHWSInstance::Release(SShaderCache *pCache, bool bReleaseData)
{
  //SAFE_DELETE(m_pSamplers);
  //SAFE_DELETE(m_pBindVars);
  //SAFE_DELETE(m_pParams[0]);
  //SAFE_DELETE(m_pParams[1]);
  //SAFE_DELETE(m_pParams_Inst);
  if (m_nParams[0] >= 0)
    CGParamManager::FreeParametersGroup(m_nParams[0]);
  if (m_nParams[1] >= 0)
    CGParamManager::FreeParametersGroup(m_nParams[1]);
  if (m_nParams_Inst >= 0)
    CGParamManager::FreeParametersGroup(m_nParams_Inst);

  int nCount = -1;
  if (m_Handle.m_pShader)
  {
    if (m_eClass == eHWSC_Pixel)
    {
      SD3DShader *pPS = m_Handle.m_pShader;
      if (pPS)
      {
        nCount = m_Handle.Release(m_eClass, m_nDataSize);
        if (!nCount && CHWShader::m_pCurPS == pPS)
          CHWShader::m_pCurPS = NULL;
      }
    }
    else
    if (m_eClass == eHWSC_Vertex)
    {
      SD3DShader *pVS = m_Handle.m_pShader;
      if (pVS)
      {
        nCount = m_Handle.Release(m_eClass, m_nDataSize);
        if (!nCount && CHWShader::m_pCurVS == pVS)
          CHWShader::m_pCurVS = NULL;
      }
    }
    else
    if (GEOMETRYSHADER_SUPPORT && m_eClass == eHWSC_Geometry)
    {
      SD3DShader *pGS = m_Handle.m_pShader;
      if (pGS)
      {
        nCount = m_Handle.Release(m_eClass, m_nDataSize);
        if (!nCount && CHWShader::m_pCurGS == pGS)
          CHWShader::m_pCurGS = NULL;
      }
    }
  }
#ifdef DIRECT3D10
  SAFE_DELETE_VOID_ARRAY(m_pShaderData);
#endif


  if (!nCount && pCache && !pCache->m_DeviceShaders.empty())
    pCache->m_DeviceShaders.erase(m_DeviceObjectID);
  m_Handle.m_pShader = NULL;
}


CHWShader_D3D::SHWSSharedList::~SHWSSharedList()
{
  int i, j;
  for (i=0; i<m_SharedInsts.size(); i++)
  {
    SHWSSharedInstance *pSInst = &m_SharedInsts[i];
    for (j=0; j<pSInst->m_Insts.size(); j++)
    {
      SHWSInstance *pInst = &pSInst->m_Insts[j];
      pInst->Release();
    }
    pSInst->m_Insts.clear();
  }
}

void CHWShader_D3D::ShutDown()
{
  int i;
  for (i=0; i<eHWSC_Max; i++)
  {
    m_PF_Params[i].clear();
    m_SG_Params[i].clear();
  }

  InstanceMapItor it;
  for (it=m_SharedInsts.begin(); it!=m_SharedInsts.end(); it++)
  {
    SHWSSharedList *pL = it->second;
    SAFE_DELETE(pL);
  }
  m_SharedInsts.clear();

  while (!m_ShaderCache.empty())
  {
    SShaderCache *pC = m_ShaderCache.begin()->second;
    SAFE_RELEASE(pC);
  }
  m_ShaderCacheList.clear();
  g_SelectedTechs.clear();
}

#if defined (DIRECT3D10) || defined(PS3)
DynArray<SCGParam> sMergedParams;
void CHWShader::MergeInstanceParams(std::vector<SCGParam> *pParams)
{
  sMergedParams.resize(0);
  int i, j, n;
  for (i=0; i<m_RegisteredFX.size(); i++)
  {
    CShader *pFX = m_RegisteredFX[i];

    for (j=0; j<pFX->m_InstParams.size(); j++)
    {
      SCGParam& pr = pFX->m_InstParams[j];
      for (n=0; n<sMergedParams.size(); n++)
      {
        SCGParam &p = sMergedParams[n];
        if (p.m_eCGParamType == pr.m_eCGParamType)
        {
          if (pr.m_dwCBufSlot > 0)
          {
            p.m_dwCBufSlot = pr.m_dwCBufSlot;
            p.m_dwBind = pr.m_dwBind;
          }
          break;
        }
      }
      if (n == sMergedParams.size())
      {
        sMergedParams.push_back(pr);
      }
    }

  }

  if (pParams)
  {
    for (i=0; i<pParams->size(); i++)
    {
      SCGParam& p = (*pParams)[i];
      if (p.m_dwCBufSlot != CB_STATIC_INSTANCE)
        continue;
      for (j=0; j<sMergedParams.size(); j++)
      {
        SCGParam& pr = sMergedParams[j];
        if (pr.m_eCGParamType == p.m_eCGParamType)
        {
          assert(pr.m_dwBind == p.m_dwBind);
          if (pr.m_dwBind == p.m_dwBind)
          {
            pr.m_dwCBufSlot = p.m_dwCBufSlot;
            pr.m_nParameters = p.m_nParameters;
          }
          break;
        }
      }
      assert(j != sMergedParams.size());
    }
  }
  uint32 nMaskCB = 0;
  for (j=0; j<sMergedParams.size(); j++)
  {
    SCGParam& pr = sMergedParams[j];
    if (pr.m_dwCBufSlot > 0)
      nMaskCB |= (1<<pr.m_dwBind);
  }

  int nInstParams = 0;
  for (j=sMergedParams.size()-1; j>=0; j--)
  {
    if (sMergedParams[j].m_dwCBufSlot > 0)
    {
      nInstParams = j + 1;
      break;
    }
  }
  for (i=0; i<m_RegisteredFX.size(); i++)
  {
    CShader *pFX = m_RegisteredFX[i];
    pFX->m_InstParams = sMergedParams;
    pFX->m_nInstParams = nInstParams;
    pFX->m_nMaskCB = nMaskCB;
  }
}

void CShader::UnregisterHW(CHWShader *pHW)
{
  if (pHW->m_eSHClass != eHWSC_Vertex)
    return;
  int i;
  for (i=0; i<pHW->m_RegisteredFX.size(); i++)
  {
    CShader *pFX = pHW->m_RegisteredFX[i];
    if (pFX == this)
    {
      pHW->m_RegisteredFX.erase(i);
      break;
    }
  }
}

void CHWShader::RegisterFX(CShader *pFX,  std::vector<SCGParam> *pParams)
{
  int i;
  for (i=0; i<m_RegisteredFX.size(); i++)
  {
    if (pFX == m_RegisteredFX[i])
      break;
  }
  if (i == m_RegisteredFX.size())
    m_RegisteredFX.push_back(pFX);
  MergeInstanceParams(pParams);
}
#endif

CHWShader *CHWShader::mfForName(const char *name, const char *nameSource, uint32 CRC32, DynArray<STexSampler>& Samplers, DynArray<SFXParam>& Params, const char *szEntryFunc, EHWShaderClass eClass, DynArray<uint32>& SHData, FXShaderToken *pTable, uint32 dwType, CShader *pFX, uint64 nMaskGen, uint64 nMaskGenFX)
{
//	LOADING_TIME_PROFILE_SECTION(iSystem);
	if (!name || !name[0])
    return NULL;

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Shader, 0, "%s", name);

  CHWShader_D3D *pSH = NULL;
	stack_string strName = name;
  CCryName className = mfGetClassName(eClass);
  stack_string AddStr;

  if (nMaskGen)
  {



    strName += AddStr.Format("(%I64x)", nMaskGen);

  }
  if (CParserBin::m_bPS3)
    strName += AddStr.Format("(P)", nMaskGen);
  else
  if (CParserBin::m_bD3D11)
    strName += AddStr.Format("(D)", nMaskGen);
  else
  if (CParserBin::m_bXenon)
    strName += AddStr.Format("(X)", nMaskGen);

  CCryName Name = strName.c_str();
  CBaseResource *pBR = CBaseResource::GetResource(className, Name, false);
  if (!pBR)
  {
    pSH = new CHWShader_D3D;
    pSH->m_NameSourceFX = nameSource;
    pSH->Register(className, Name);
    pSH->m_EntryFunc = szEntryFunc;
    pSH->mfFree(CRC32);
#if defined (DIRECT3D10) || defined(PS3)
    if (eClass == eHWSC_Vertex)
      pSH->RegisterFX(pFX, NULL);
#endif
  }
  else
  {
    pSH = (CHWShader_D3D *)pBR;
    pSH->AddRef();
#if defined (DIRECT3D10) || defined(PS3)
    if (eClass == eHWSC_Vertex)
      pSH->RegisterFX(pFX, NULL);
#endif
    if (pSH->m_CRC32 == CRC32)
    {
      if (pTable && gRenDev->m_cEF.m_eCacheMode != eSC_BuildPerLevel)
      {
        FXShaderToken *pMap = pTable;
        DynArray<uint32> *pData = &SHData;
        pSH->mfGetCacheTokenMap(pMap, pData, pSH->m_nMaskGenShader);
      }
      return pSH;
    }
    pSH->mfFree(CRC32);
    pSH->m_CRC32 = CRC32;
  }

  if (CParserBin::m_bEditable)
  {
    if (pTable)
      pSH->m_Table = *pTable;
    pSH->m_TokenData = SHData;
  }

  /*if (pTable)
  {
    TArray<char> sNewScr;
    CParserBin::ConvertToAscii(&SHData[0], SHData.size(), *pTable, sNewScr);
  #undef fopen
    FILE *fp = fopen(SYS_APP_HOME"/test.txt", "w");
  #define fopen WrappedFopen
    if (fp)
    {
      gEnv->pCryPak->FPrintf(fp, "%s", &sNewScr[0]);
      gEnv->pCryPak->FClose (fp);
    }
  }*/

  pSH->m_dwShaderType = dwType;
  pSH->m_eSHClass = eClass;
  pSH->m_nMaskGenShader = nMaskGen;
  pSH->m_nMaskGenFX = nMaskGenFX;
  pSH->m_Samplers = Samplers;
  pSH->m_Params = Params;
  pSH->m_CRC32 = CRC32;

  pSH->mfConstructFX(pTable, &SHData);

  return pSH;
}


void CHWShader_D3D::SetTokenFlags(uint32 nToken)
{
  switch (nToken)
  {
  case eT__LT_LIGHTS:
    m_Flags |= HWSG_SUPPORTS_LIGHTING;
    break;
  case eT__LT_0_TYPE:
  case eT__LT_1_TYPE:
  case eT__LT_2_TYPE:
  case eT__LT_3_TYPE:
    m_Flags |= HWSG_SUPPORTS_MULTILIGHTS;
    break;
  case eT__TT0_TCM:
  case eT__TT1_TCM:
  case eT__TT2_TCM:
  case eT__TT3_TCM:
  case eT__TT0_TCG_TYPE:
  case eT__TT1_TCG_TYPE:
  case eT__TT2_TCG_TYPE:
  case eT__TT3_TCG_TYPE:
  case eT__TT0_TCPROJ:
  case eT__TT1_TCPROJ:
  case eT__TT2_TCPROJ:
  case eT__TT3_TCPROJ:
  case eT__TT0_TCUBE:
  case eT__TT1_TCUBE:
  case eT__TT2_TCUBE:
  case eT__TT3_TCUBE:
    m_Flags |= HWSG_SUPPORTS_MODIF;
    break;
  case eT__VT_TYPE:
    m_Flags |= HWSG_SUPPORTS_VMODIF;
    break;
  case eT__FT_TEXTURE:
    m_Flags |= HWSG_FP_EMULATION;
    break;
  }
}

uint64 CHWShader_D3D::CheckToken(uint32 nToken)
{
  uint64 nMask = 0;
  SShaderGen *pGen = gRenDev->m_cEF.m_pGlobalExt;
  uint32 i;
  for (i=0; i<pGen->m_BitMask.Num(); i++)
  {
    SShaderGenBit *bit = pGen->m_BitMask[i];
    if (!bit)
      continue;

    if (bit->m_dwToken == nToken)
    {
      nMask |= bit->m_Mask;
      break;
    }
  }
  if (!nMask)
    SetTokenFlags(nToken);

  return nMask;
}

uint64 CHWShader_D3D::CheckIfExpr_r(uint32 *pTokens, uint32& nCur, uint32 nSize)
{
  uint64 nMask = 0;

  while (nCur < nSize)
  {
    int nRecurs = 0;
    uint32 nToken = pTokens[nCur++];
    if (nToken == eT_br_rnd_1) // check for '('
    {
      uint32 tmpBuf[64];
      int n = 0;
      int nD = 0;
      while (true)
      {
        nToken = pTokens[nCur];
        if (nToken == eT_br_rnd_1) // check for '('
          n++;
        else
        if (nToken == eT_br_rnd_2) // check for ')'
        {
          if (!n)
          {
            tmpBuf[nD] = 0;
            nCur++;
            break;
          }
          n--;
        }
        else
        if (nToken == 0)
          return nMask;
        tmpBuf[nD++] = nToken;
        nCur++;
      }
      if (nD)
      {
        uint32 nC = 0;
        nMask |= CheckIfExpr_r(tmpBuf, nC, nSize);
      }
    }
    else
    {
      bool bNeg = false;
      if (nToken == eT_excl)
      {
        bNeg = true;
        nToken = pTokens[nCur++];
      }
      nMask |= CheckToken(nToken);
    }
    nToken = pTokens[nCur];
    if (nToken == eT_or)
    {
      nCur++;
      assert (pTokens[nCur] == eT_or);
      if (pTokens[nCur] == eT_or)
        nCur++;
    }
    else
    if (nToken == eT_and)
    {
      nCur++;
      assert (pTokens[nCur] == eT_and);
      if (pTokens[nCur] == eT_and)
        nCur++;
    }
    else
      break;
  }
  return nMask;
}

void CHWShader_D3D::mfConstructFX_Mask_RT(FXShaderToken *Table, DynArray<uint32>* pSHData)
{
  assert(gRenDev->m_cEF.m_pGlobalExt);
  m_nMaskAnd_RT = 0;
  m_nMaskOr_RT = 0;
  if (!gRenDev->m_cEF.m_pGlobalExt)
    return;
  SShaderGen *pGen = gRenDev->m_cEF.m_pGlobalExt;

  /*if (!stricmp(m_EntryFunc.c_str(), "TerrainVS"))
  {
    int nnn = 0;
  }*/

  if (m_Flags & HWSG_SHARED)
  {
    assert(!pSHData->empty());
    uint32 *pTokens = &(*pSHData)[0];
    uint32 nSize = pSHData->size();
    uint32 nCur = 0;
    while (nCur < nSize)
    {
      uint32 nTok = CParserBin::NextToken(pTokens, nCur, nSize-1);
      if (!nTok)
        continue;
      if (nTok < eT_if || nTok > eT_elif)
        SetTokenFlags(nTok);
    }

    if (m_dwShaderType)
    {
      for (uint32 i=0; i<pGen->m_BitMask.Num(); i++)
      {
        SShaderGenBit *bit = pGen->m_BitMask[i];
        if (!bit)
          continue;
        if (bit->m_Flags & SHGF_RUNTIME)
        {
          m_nMaskAnd_RT |= bit->m_Mask;
          continue;
        }
        uint32 j;
        if (bit->m_PrecacheNames.size())
        {
          for (j=0; j<bit->m_PrecacheNames.size(); j++)
          {
            if (m_dwShaderType == bit->m_PrecacheNames[j])
            {
              m_nMaskAnd_RT |= bit->m_Mask;
              break;
            }
          }
        }
      }
    }
    mfSetDefaultRT(m_nMaskAnd_RT, m_nMaskOr_RT);
  }
  else
  {
    assert(!pSHData->empty());
    uint32 *pTokens = &(*pSHData)[0];
    uint32 nSize = pSHData->size();
    uint32 nCur = 0;
    while (nCur < nSize)
    {
      uint32 nTok = CParserBin::NextToken(pTokens, nCur, nSize-1);
      if (!nTok)
        continue;
      if (nTok >= eT_if && nTok <= eT_elif)
        m_nMaskAnd_RT |= CheckIfExpr_r(pTokens, nCur, nSize);
      else
        SetTokenFlags(nTok);
    }

    // Reset any RT bits for this shader if this shader type is not existing for specific bit
    // See Runtime.ext file
    if (m_dwShaderType)
    {
      for (uint32 i=0; i<pGen->m_BitMask.Num(); i++)
      {
        SShaderGenBit *bit = pGen->m_BitMask[i];
        if (!bit)
          continue;
        uint32 j;
        if (bit->m_PrecacheNames.size())
        {
          for (j=0; j<bit->m_PrecacheNames.size(); j++)
          {
            if (m_dwShaderType == bit->m_PrecacheNames[j])
              break;
          }
          if (j == bit->m_PrecacheNames.size())
            m_nMaskAnd_RT &= ~bit->m_Mask;
        }
        else
          m_nMaskAnd_RT &= ~bit->m_Mask;
      }
    }
    mfSetDefaultRT(m_nMaskAnd_RT, m_nMaskOr_RT);
  }
}

void CHWShader_D3D::mfConstructFX(FXShaderToken* Table, DynArray<uint32>* pSHData)
{
  if (!strnicmp(m_EntryFunc.c_str(), "Common_", 7))
    m_Flags |= HWSG_SHARED;
  if (!strnicmp(m_EntryFunc.c_str(), "Sync_", 5))
    m_Flags |= HWSG_SYNC;

  if (!pSHData->empty())
    mfConstructFX_Mask_RT(Table, pSHData);
  else
  {
    m_nMaskAnd_RT = -1;
    m_nMaskOr_RT = 0;
  }

  if (Table && gRenDev->m_cEF.m_eCacheMode != eSC_BuildPerLevel)
  {
    FXShaderToken *pMap = Table;
    DynArray<uint32> *pData = pSHData;
    mfGetCacheTokenMap(pMap, pData, m_nMaskGenShader);   // Store tokens
  }
}

bool CHWShader_D3D::mfPrecache(SShaderCombination& cmb, bool bForce)
{
  bool bRes = true;

  if (CRenderer::CV_r_shadersnocompile)
    return bRes;

  /*if (!stricmp(m_EntryFunc.c_str(), "ParticleVS"))
  {
    int nnn = 0;
  }*/
  uint64 AndRTMask = 0;
  uint64 OrRTMask = 0;
  mfSetDefaultRT(AndRTMask, OrRTMask);
  uint64 RTMask = cmb.m_RTMask & AndRTMask | OrRTMask;
  uint32 LTMask = 0;
  uint32 nMDMask = 0;
  uint32 nMDVMask = cmb.m_MDVMask;
  if (m_eSHClass == eHWSC_Pixel)
    nMDVMask = 0;
  if (m_Flags & HWSG_SUPPORTS_MULTILIGHTS)
    LTMask = 1;
  uint64 GLMask = m_nMaskGenShader;
  uint32 nFlags = HWSF_PRECACHE;
  if (m_eSHClass == eHWSC_Pixel && gRenDev->m_RP.m_pShaderResources)
  {
    SHWSInstance *pInst = mfGetInstance(RTMask, LTMask, GLMask, nMDMask, nMDVMask, HWSF_PRECACHE_INST);
    int nResult = mfCheckActivation(pInst, HWSF_PRECACHE);
    if (!nResult)
      return bRes;
    mfUpdateSamplers();
    pInst->m_fLastAccess = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;
    nMDMask = gRenDev->m_RP.m_FlagsShader_MD & ~HWMD_TCMASK;
  }
  if (m_eSHClass == eHWSC_Pixel && gRenDev->m_RP.m_pShaderResources)
    nMDMask &= ~HWMD_TCMASK;

  if (nMDMask) // || bForce)
  {
    SHWSInstance *pInst = mfGetInstance(RTMask, LTMask, GLMask, nMDMask, nMDVMask, HWSF_PRECACHE_INST);
    //pInst->m_bFallback = bForce;
    pInst->m_fLastAccess = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;
    mfActivate(nFlags);
  }

  return bRes;
}



//==============================================================================================

DynArray<SCGParamPool> CGParamManager::m_Pools;
DynArray<SCGParamsGroup> CGParamManager::m_Groups;
DynArray<uint32> CGParamManager::m_FreeGroups;

SCGParamsGroup SCGParamPool::Alloc(int nEntries)
{
  SCGParamsGroup Group;

  alloc_info_struct *pAI = gRenDev->GetFreeChunk(nEntries, m_nMaxEntries, m_alloc_info, "CGParam");
  if (pAI)
  {
    Group.nParams = nEntries;
    Group.pParams = &m_Params[pAI->ptr];
  }

  return Group;
}

bool SCGParamPool::Free(SCGParamsGroup& Group)
{
  bool bRes = gRenDev->ReleaseChunk(Group.pParams-&m_Params[0], m_alloc_info);
  return bRes;
}

int CGParamManager::GetParametersGroup(std::vector<SCGParam>& InParams)
{
  int i;
  int nParams = InParams.size();

  for (i=0; i<m_Groups.size(); i++)
  {
    SCGParamsGroup& Gr = m_Groups[i];
    if (Gr.nParams != nParams)
      continue;
    int j;
    for (j=0; j<nParams; j++)
    {
      if (InParams[j] != Gr.pParams[j])
        break;
    }
    if (j == nParams)
    {
      Gr.nRefCounter++;
      return i;
    }
  }

  SCGParamsGroup Group;
  SCGParamPool *pPool = NULL;
  for (i=0; i<m_Pools.size(); i++)
  {
    pPool = &m_Pools[i];
    Group = pPool->Alloc(nParams);
    if (Group.nParams)
      break;
  }
  if (!Group.pParams)
  {
    pPool = NewPool(PARAMS_POOL_SIZE);
    Group = pPool->Alloc(nParams);
  }
  assert(Group.pParams);
  if (!Group.pParams)
    return 0;
  Group.nPool = i;
  uint32 n = m_Groups.size();
  if (m_FreeGroups.size())
  {
    int nID = m_FreeGroups.size()-1;
    n = m_FreeGroups[nID];
    m_FreeGroups.erase(m_FreeGroups.begin()+nID);
    m_Groups[n] = Group;
  }
  else
  {
    m_Groups.push_back(Group);
  }

  for (i=0; i<nParams; i++)
  {
    m_Groups[n].pParams[i] = InParams[i];
  }

  return n;
}

bool CGParamManager::FreeParametersGroup(int nIDGroup)
{
  assert(nIDGroup>=0 && nIDGroup<m_Groups.size());
  if (nIDGroup<0 || nIDGroup>=m_Groups.size())
    return false;
  SCGParamsGroup& Group = m_Groups[nIDGroup];
  Group.nRefCounter--;
  if (Group.nRefCounter)
    return true;
  assert(Group.nPool>=0 && Group.nPool<m_Pools.size());
  if (Group.nPool<0 || Group.nPool>=m_Pools.size())
    return false;
  SCGParamPool& Pool = m_Pools[Group.nPool];
  if (!Pool.Free(Group))
    return false;
  for (int i=0; i<Group.nParams; i++)
  {
    Group.pParams[i].m_Name.reset();
    SAFE_DELETE(Group.pParams[i].m_pData);
  }

	Group.nParams = 0;
	Group.nPool = 0;
	Group.pParams = 0;

  m_FreeGroups.push_back(nIDGroup);

  return true;
}

SCGParamPool *CGParamManager::NewPool(int nEntries)
{
  SCGParamPool Pool;
  Pool.m_nMaxEntries = nEntries;
  Pool.m_Params.resize(nEntries);
  m_Pools.push_back(Pool);

  return &m_Pools[m_Pools.size()-1];
}

//===========================================================================================================

const SWaveForm sWFX = SWaveForm(eWF_Sin, 0, 3.5f, 0, 0.2f);
const SWaveForm sWFY = SWaveForm(eWF_Sin, 0, 5.0f, 90.0f, 0.2f);

union UFloat4
{
  float f[4];


#if defined(_CPU_SSE)
  __m128 m128;
#endif
};

DEFINE_ALIGNED_DATA(UFloat4, sData[32], 16);
DEFINE_ALIGNED_DATA(float, sTempData[32][4], 16);
DEFINE_ALIGNED_DATA(float, sMatrInstData[3][4], 16);
/*
void sTranspose(Matrix34A& m, Matrix44A *dst)
{
  dst->m00=m.m00;	dst->m01=m.m10;	dst->m02=m.m20;	dst->m03=0;
  dst->m10=m.m01;	dst->m11=m.m11;	dst->m12=m.m21;	dst->m13=0;
  dst->m20=m.m02;	dst->m21=m.m12;	dst->m22=m.m22;	dst->m23=0;
  dst->m30=m.m03;	dst->m31=m.m13;	dst->m32=m.m23;	dst->m33=1;
}
void sTranspose(Matrix34A& m, Matrix33 *dst)
{
  dst->m00=m.m00;	dst->m01=m.m10;	dst->m02=m.m20;
  dst->m10=m.m01;	dst->m11=m.m11;	dst->m12=m.m21;
  dst->m20=m.m02;	dst->m21=m.m12;	dst->m22=m.m22;
}
*/
namespace
{
  NO_INLINE void sIdentityLine()
  {
#ifndef XENON_INTRINSICS
    sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = 0.f; sData[0].f[3] = 1.0f;


#endif
  }
  NO_INLINE void sOneLine()
  {
#ifndef XENON_INTRINSICS
    sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = 1.f; sData[0].f[3] = 1.0f;


#endif
  }
  NO_INLINE void sZeroLine()
  {
#ifndef XENON_INTRINSICS
    sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = 0.f; sData[0].f[3] = 0.0f;


#endif
  }



  NO_INLINE float* sGetLightMatrix(CD3D9Renderer *r)
  {
    SLightPass *pLP = &r->m_RP.m_LPasses[r->m_RP.m_nCurLightPass];
    assert (pLP->nLights==1 && (pLP->pLights[0]->m_Flags & DLF_PROJECT));
    CDLight *pDL = pLP->pLights[0];
    if (pDL && pDL->m_pLightImage)
    {
      Matrix44 ProjMatrixT;
      CShadowUtils::GetProjectiveTexGen(pDL, 0, &ProjMatrixT);

      //*(Matrix44 *)&sData[0].f[0] = pDL->m_ProjMatrix;
      ((Matrix44A *)&sData[0])->Transpose(ProjMatrixT);

    }
    else
      ((Matrix44A *)&sData[0])->SetIdentity();
    return &sData[0].f[0];
  }
  NO_INLINE CRendElementBase *sGetContainerRE0(CRendElementBase * pRE)
  {
    assert(pRE);		// someone assigned wrong shader - function should not be called then

    if(pRE->mfGetType() == eDATA_Mesh && ((CREMesh*)pRE)->m_pRenderMesh->_GetVertexContainer())
    {
      assert(((CREMesh*)pRE)->m_pRenderMesh->_GetVertexContainer()->m_Chunks.Count()>=1);
      return ((CREMesh*)pRE)->m_pRenderMesh->_GetVertexContainer()->m_Chunks[0].pRE;
    }

    return pRE;
  }

  NO_INLINE float *sGetTerrainBase(CD3D9Renderer *r)
  {
    if(!r->m_RP.m_pRE)  
      return NULL;				// it seems the wrong material was assigned 

    // use render element from vertex container render mesh if available
    CRendElementBase *pRE = sGetContainerRE0(r->m_RP.m_pRE);

    if (pRE->m_CustomData)
    {
      float *pData;

      if(SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID]<=1)
        pData = (float *)pRE->m_CustomData;
      else
        pData = (float *)pRE->m_CustomData + 4;

      sData[0].f[0] = pData[2]; sData[0].f[1] = pData[0]; sData[0].f[2] = pData[1]; sData[0].f[3] = gEnv->p3DEngine->GetTerrainTextureMultiplier();
      
    }
		else
			sZeroLine();

    return &sData[0].f[0];
  }
  NO_INLINE float *sGetTerrainLayerGen(CD3D9Renderer *r)
  {
    if(!r->m_RP.m_pRE)
      return NULL;				// it seems the wrong material was assigned 

    CRendElementBase *pRE = r->m_RP.m_pRE;

    float *pData = (float *)pRE->m_CustomData;
    if (pData)
    {
      sData[0].f[0] = pData[0]; sData[0].f[1] = pData[1]; sData[0].f[2] = pData[2]; sData[0].f[3] = pData[3];
      sData[1].f[0] = pData[4]; sData[1].f[1] = pData[5]; sData[1].f[2] = pData[6]; sData[1].f[3] = pData[7];
      sData[2].f[0] = pData[8]; sData[2].f[1] = pData[9]; sData[2].f[2] = pData[10]; sData[2].f[3] = pData[11];
      sData[3].f[0] = pData[12]; sData[3].f[1] = pData[13]; sData[3].f[2] = pData[14]; sData[3].f[3] = pData[15];
      return &sData[0].f[0];
    }
    else
      return NULL;
  }
  NO_INLINE float *sGetVoxTerrainAtlasInfo(CD3D9Renderer *r)
  {
    if(!r->m_RP.m_pRE)
      return NULL;

    CRendElementBase *pRE = r->m_RP.m_pRE;

    float *pData = (float *)pRE->m_CustomData;
    if (pData)
    {
      sData[0].f[0] = pData[0];   sData[0].f[1] = pData[1];   sData[0].f[2] = pData[2];   sData[0].f[3] = pData[3];
      sData[1].f[0] = pData[4];   sData[1].f[1] = pData[5];   sData[1].f[2] = pData[6];   sData[1].f[3] = pData[7];
      sData[2].f[0] = pData[8];   sData[2].f[1] = pData[9];   sData[2].f[2] = pData[10];  sData[2].f[3] = pData[11];
      sData[3].f[0] = pData[12];  sData[3].f[1] = pData[13];  sData[3].f[2] = pData[14];  sData[3].f[3] = pData[15];
      sData[4].f[0] = pData[16];  sData[4].f[1] = pData[17];  sData[4].f[2] = pData[18];  sData[4].f[3] = pData[19];
      sData[5].f[0] = pData[20];  sData[5].f[1] = pData[21];  sData[5].f[2] = pData[22];  sData[5].f[3] = pData[23];
      return &sData[0].f[0];
    }
    else
      return NULL;
  }
  float *sGetTexMatrix(CD3D9Renderer *r, const SCGParam *ParamBind)
  {
    static int nLastObjFrame=-1;
    static Vec3 pLastPos;
    static Ang3 pLastAngs;
    static CTexture *pLastTex;
    DEFINE_ALIGNED_DATA_STATIC(Matrix44, m, 16);

    CTexture *tp = NULL;
    SHRenderTarget *pTarg = (SHRenderTarget *)(UINT_PTR)ParamBind->m_nID;
    assert(pTarg);
    if (!pTarg)
      return NULL;
    SEnvTexture *pEnvTex = pTarg->GetEnv2D();
    //assert(pEnvTex && pEnvTex->m_pTex);
    if (!pEnvTex || !pEnvTex->m_pTex)
      return NULL;
    if (r->m_RP.m_FrameObject != nLastObjFrame || pLastTex != tp || (!r->GetCamera().GetAngles().IsEquivalent(pLastAngs,VEC_EPSILON)) || (!IsEquivalent(r->GetCamera().GetPosition(),pLastPos,VEC_EPSILON)))
    {
      pLastTex = tp;
      pLastPos = r->GetCamera().GetPosition();
      pLastAngs = r->GetCamera().GetAngles();
      nLastObjFrame = r->m_RP.m_FrameObject;
      
      if ((pTarg->m_eUpdateType != eRTUpdate_WaterReflect) && r->m_RP.m_pCurObject->m_ObjFlags & FOB_TRANS_MASK)
        m = r->m_RP.m_pCurObject->m_II.m_Matrix * pEnvTex->m_Matrix;
      else
        m = pEnvTex->m_Matrix;

      m.Transpose();
    }
    return m.GetData();
  }
  void sGetWind(CD3D9Renderer *r)
  {
    static int nLastObjFrame=-1;    
    static Vec4 pWind( 0, 0, 0, 0 );    

    sData[0].f[0] = 0.0f;
    sData[0].f[1] = 0.0f;
    sData[0].f[2] = 0.0f;
    sData[0].f[3] = 0.0f;    

    if ( r->m_RP.m_FrameObject != nLastObjFrame )
    {
      nLastObjFrame = r->m_RP.m_FrameObject;

      CRenderObject *pObj = r->m_RP.m_pCurObject;
      SBending *pB = pObj->m_pBending;
      //assert(pB);
      if (!pB)
        return;
      pWind.x = pB->m_vBending.x;
      pWind.y = pB->m_vBending.y;
      
      // Get phase variation based on object id
      pWind.z = (float) ((int) pObj->m_pRenderNode)/ (float) (INT_MAX);
      pWind.z *= 100000.0f;
      pWind.z -= floorf( pWind.z );
      pWind.z *= 10.0f;

      pWind.w = pB->m_vBending.GetLength();
    }

    sData[0].f[0] = pWind.x;
    sData[0].f[1] = pWind.y;
    sData[0].f[2] = pWind.z;
    sData[0].f[3] = pWind.w;    
  }
  NO_INLINE void sGetRotGridScreenOff(CD3D9Renderer *r)
  {
    int iTempX, iTempY, iWidth, iHeight;
    r->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
    sData[0].f[0] = 1.0f / (float)iWidth;
    sData[0].f[1] = 0.f; 
    sData[0].f[2] = 0.f;
    sData[0].f[3] = 1.0f / (float)iHeight;
    //rotated grid    
    Vec4 t75 = Vec4(0.75f * sData[0].f[0], 0.75f * sData[0].f[1], 0.75f * sData[0].f[2], 0.75f * sData[0].f[3]);
    Vec4 t25 = Vec4(0.25f * sData[0].f[0], 0.25f * sData[0].f[1], 0.25f * sData[0].f[2], 0.25f * sData[0].f[3]);
    Vec2 rotX = Vec2(t75[0]+t25[2], t75[1]+t25[3]);
    Vec2 rotY = Vec2(t75[2]-t25[0], t75[3]-t25[1]); 
    sData[0].f[0] = rotX[0];     sData[0].f[1] = rotX[1];
    sData[0].f[2] = rotY[0];     sData[0].f[3] = rotY[1];
  }

  NO_INLINE float sGetMaterialLayersOpacity( CD3D9Renderer *r )
  {
    float fMaterialLayersOpacity = 1.0f;

    uint32 nResourcesNoDrawFlags = r->m_RP.m_pShaderResources->GetMtlLayerNoDrawFlags();
    if( (r->m_RP.m_pCurObject->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK) && !(nResourcesNoDrawFlags&MTL_LAYER_CLOAK) )
    {
      fMaterialLayersOpacity = ((float)((r->m_RP.m_pCurObject->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK)>> 8) / 255.0f);
      fMaterialLayersOpacity = min(1.0f, 4.0f * max( 1.0f - fMaterialLayersOpacity, 0.0f) ); 
    }

    return fMaterialLayersOpacity;
  }

  NO_INLINE void sGetScreenSize(CD3D9Renderer *r)
  {
    int iTempX, iTempY, iWidth, iHeight;
    r->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
    sData[0].f[0] = (float)iWidth;
    sData[0].f[1] = (float)iHeight;
    sData[0].f[2] = 0.5f/(float)iWidth;
    sData[0].f[3] = 0.5f/(float)iHeight;
  }
  NO_INLINE void sGetIrregKernel(CD3D9Renderer *r)
  {
#define PACKED_SAMPLES 1
    //samples for cubemaps
    /*const Vec4 irreg_kernel[8]=
    {
      Vec4(0.527837f, -0.085868f, 0.527837f, 0),
      Vec4(-0.040088f, 0.536087f, -0.040088f, 0),
      Vec4(-0.670445f, -0.179949f, -0.670445f, 0),
      Vec4(-0.419418f, -0.616039f, -0.419418f, 0),
      Vec4(0.440453f, -0.639399f, 0.440453f, 0),
      Vec4(-0.757088f, 0.349334f, -0.757088f, 0),
      Vec4(0.574619f, 0.685879f, 0.574619f, 0),
      Vec4(0.03851f, -0.939059f, 0.03851f, 0)
    };
    f32 fFrustumScale = r->m_cEF.m_TempVecs[4][0]; //take only first cubemap 
    for (int i=0; i<8; i++)
    {
      sData[i].f[0] = irreg_kernel[i][0] * (1.0f/fFrustumScale);
      sData[i].f[1] = irreg_kernel[i][1] * (1.0f/fFrustumScale);
      sData[i].f[2] = irreg_kernel[i][2] * (1.0f/fFrustumScale);
      sData[i].f[3] = 0;
    }*/

    int nSamplesNum = 1;
    switch (r->m_RP.m_nShaderQuality)
    {
      case eSQ_Medium:
      case eSQ_High:
        nSamplesNum = 8;
        break;
      case eSQ_VeryHigh:
        nSamplesNum = 16;
        break;
    }

    CPoissonDiskGen::SetKernelSize(nSamplesNum);

#ifdef PACKED_SAMPLES
    for (int i=0, nIdx=0; i<nSamplesNum; i+=2, nIdx++)
    {
      Vec2 vSample = CPoissonDiskGen::GetSample(i);
      sData[nIdx].f[0] = vSample.x;
      sData[nIdx].f[1] = vSample.y;
      vSample = CPoissonDiskGen::GetSample(i+1);
      sData[nIdx].f[2] = vSample.x;
      sData[nIdx].f[3] = vSample.y;
    }
#else
    for (int i=0, nIdx=0; i<nSamplesNum; i++, nIdx++)
    {
      Vec2 vSample = CPoissonDiskGen::GetSample(i); 
      sData[nIdx].f[0] = vSample.x;
      sData[nIdx].f[1] = vSample.y;
      sData[nIdx].f[2] = 0.0f;
      sData[nIdx].f[3] = 0.0f;
    }
#endif

#undef PACKED_SAMPLES

  }

  NO_INLINE void sGetRegularKernel(CD3D9Renderer *r)
  {

    float fRadius = r->CV_r_shadow_jittering;
    float SHADOW_SIZE = 1024.f;

    const Vec4 regular_kernel[9]=
    {
      Vec4(-1, 1, 0, 0),
      Vec4( 0, 1, 0, 0),
      Vec4( 1, 1, 0, 0),
      Vec4(-1, 0, 0, 0),
      Vec4( 0, 0, 0, 0),
      Vec4( 1, 0, 0, 0),
      Vec4(-1, -1, 0, 0),
      Vec4( 0, -1, 0, 0),
      Vec4( 1, -1, 0, 0)
    };

    float fFilterRange = fRadius/SHADOW_SIZE;

    for (int32 nInd = 0; nInd<9; nInd++)
    {
        if ((nInd%2) == 0)
        {
          sData[nInd/2].f[0] = regular_kernel[nInd].x * fFilterRange;
          sData[nInd/2].f[1] = regular_kernel[nInd].y * fFilterRange;
        }
        else
        {
          sData[nInd/2].f[2] = regular_kernel[nInd].x * fFilterRange;;
          sData[nInd/2].f[3] = regular_kernel[nInd].y * fFilterRange;;
        }
    }

    /*float FilterRange = radius/SHADOW_SIZE;
    float FilterStep = 1.0f/SHADOW_SIZE;

    int32 nInd = 0;
    for (float y=-FilterRange; y<FilterRange; y+=FilterStep)
    {
      for (float x=-FilterRange; x<FilterRange; x+=FilterStep, ++nInd)
      {
        assert((nInd/2) <= 5);

        if ((nInd%2) == 0)
        {
          sData[nInd/2][0] = x;
          sData[nInd/2][1] = y;
        }
        else
        {
          sData[nInd/2][2] = x;
          sData[nInd/2][3] = y;
        }
      }
    }
    */
  }
  NO_INLINE void sGetBendInfo(CD3D9Renderer *r)
  {
    static int nLastObjFrame=-1;    
    static Vec4 vLastBending( 0, 0, 0, 0 );

    const SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    if (rRP.m_FrameObject != nLastObjFrame)
    {
      nLastObjFrame = rRP.m_FrameObject;

			Vec4 vCurBending(0, 0, 0, 0);
      const CRenderObject * const __restrict pObj = rRP.m_pCurObject;
      
      const SBending *const __restrict pB = pObj->m_pBending;
      assert(pB);

      Vec2 vBending = pB->m_vBending;
      float fBendScale = pB->m_fBendScale;

      if ( !( pObj->m_ObjFlags & (FOB_SHADERLOD0)) && CRenderer::CV_r_ShaderLod )
      {
        // save 1 vs permutation - do transition in cpu instead
        static ICVar* const __restrict e_LodDistShader( gEnv->pConsole->GetCVar( "e_LodDistShader" ) );
        float fLodDistShader = e_LodDistShader->GetFVal();
        float fLodAtten = 1.0f - min(1.0f, pObj->m_fDistance / fLodDistShader);          
        vBending *= fLodAtten;
        fBendScale *=fLodAtten;
      }

      if (fabs_tpl(vBending.x + vBending.y + fBendScale) > 0.0f)
      {
        if (fBendScale == 0.0f)
        {          
          vCurBending.z = vBending.GetLength();
          vCurBending.x = vCurBending.y = vCurBending.w = 0.0f;
        }
        else
        {
          // Wind affected bending
          vCurBending.x = vBending.x;
          vCurBending.y = vBending.y;
          vCurBending.z = fBendScale;
        }

        if (pB->m_Waves[0].m_Amp)
        {
					const float realTime = rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime;
					// Fast version of CShaderMan::EvalWaveForm (for bending)
					const SWaveForm2& RESTRICT_REFERENCE wave0 = pB->m_Waves[0];
					const SWaveForm2& RESTRICT_REFERENCE wave1 = pB->m_Waves[1];
				  int val0 = (int)((realTime*wave0.m_Freq+wave0.m_Phase)*(float)SRenderPipeline::sSinTableCount);
			    int val1 = (int)((realTime*wave1.m_Freq+wave1.m_Phase)*(float)SRenderPipeline::sSinTableCount);
					float sinVal0 = rRP.m_tSinTable[val0&(SRenderPipeline::sSinTableCount-1)];
					float sinVal1 = rRP.m_tSinTable[val1&(SRenderPipeline::sSinTableCount-1)];
					float bendAdd0 = wave0.m_Amp*sinVal0+wave0.m_Level;
					float bendAdd1 = wave1.m_Amp*sinVal1+wave1.m_Level;
          vCurBending.x += bendAdd0;
          vCurBending.y += bendAdd1;
        }

        if (pB->m_fBendScale == 0.0f)
        {
          // This values are getting scaled down in shader (due to wind version), rescale them back
          vCurBending.x *= 50.0f;
          vCurBending.y *= 50.0f;

          vCurBending.w = vCurBending.z * 0.25f; // Must scale down wind strength (due to wind version)
        }
        else
        {
          vCurBending.w = Vec2(vCurBending.x, vCurBending.y).GetLength();
        }
      }
      else
      {
        vCurBending(0,0,0,0);
      }
			*(Vec4*)&sData[0] = vCurBending;
			vLastBending			= vCurBending;
    }
		else
			*(Vec4*)&sData[0] = vLastBending;
  }

  NO_INLINE Vec4 sGetVolumetricFogParams(CD3D9Renderer *r)
  {
    static int nFrameID = 0;
    static Vec4 pFogParams = Vec4(0,0,0,0);

    //if( nFrameID != gRenDev->GetFrameID() )
    {
      I3DEngine *pEng = gEnv->p3DEngine;  

      float globalDensity(0); float atmosphereHeight(0); float artistTweakDensityOffset(0); float globalDensityMultiplierLDR(1);
      pEng->GetVolumetricFogSettings(globalDensity, atmosphereHeight, artistTweakDensityOffset, globalDensityMultiplierLDR);

      float globalDensityMod(0); float atmosphereHeightMod(0);
      pEng->GetVolumetricFogModifiers(globalDensityMod, atmosphereHeightMod);

      globalDensity += globalDensityMod;		
      globalDensity	= ( globalDensity < 0.0f ) ? 0.0f : globalDensity;
      atmosphereHeight += atmosphereHeightMod;
      atmosphereHeightMod = ( atmosphereHeight < 1.0f ) ? 1.0f : atmosphereHeight;

			if (!gRenDev->EF_Query(EFQ_HDRModeEnabled))
				globalDensity *= globalDensityMultiplierLDR;

      float atmosphereScale( 16.0f / atmosphereHeight ); // used as an argument for exp( -a * x ); if x >= AtmosphereHeight then the density for the given height will be close to zero
      float viewerHeight(r->GetRCamera().Orig.z);
      float waterLevel(pEng->GetWaterLevel());
      if( fabsf( waterLevel - WATER_LEVEL_UNKNOWN ) < 1e-4 )
        waterLevel = 0.0f;

      globalDensity *= 0.01f; // multiply by 1/100 to scale value editor value back to a reasonable range

      pFogParams.x = atmosphereScale;
      pFogParams.y = 1.44269502f * globalDensity * expf( -atmosphereScale * ( viewerHeight - waterLevel ) ); // log2(e) = 1.44269502
      pFogParams.z = globalDensity; 
      pFogParams.w = artistTweakDensityOffset; 

      nFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    }
    sData[0].f[0] = pFogParams.x; 
    sData[0].f[1] = pFogParams.y; 
    sData[0].f[2] = pFogParams.z; 
    sData[0].f[3] = pFogParams.w;

    return pFogParams;
  }

	NO_INLINE Vec4 sGetVolumetricFogRampParams()
	{
		I3DEngine *pEng = gEnv->p3DEngine;
		
		Vec3 vfRampParams(0, 100.0f, 0);
		pEng->GetGlobalParameter(E3DPARAM_VOLFOG_RAMP, vfRampParams);

		vfRampParams.x = vfRampParams.x < 0 ? 0 : vfRampParams.x; // start
		vfRampParams.y = vfRampParams.y < vfRampParams.x + 0.1f ? vfRampParams.x + 0.1f : vfRampParams.y; // end
		vfRampParams.z = clamp_tpl(vfRampParams.z, 0.0f, 1.0f); // influence

		float invRampDist = 1.0f / (vfRampParams.y - vfRampParams.x);
		return Vec4(invRampDist, -vfRampParams.x * invRampDist, vfRampParams.z, -vfRampParams.z + 1.0f);
	}

	NO_INLINE void sGetFogColorGradientConstanst(Vec4& fogColGradColBase, Vec4& fogColGradColDelta)
	{
		I3DEngine* pEng = gEnv->p3DEngine;

		Vec3 colBase = pEng->GetFogColor();
		fogColGradColBase = Vec4(colBase, 0);

		Vec3 colTop(0, 0, 0);
		pEng->GetGlobalParameter(E3DPARAM_FOG_COLOR2, colTop);
		fogColGradColDelta = Vec4(colTop - colBase, 0);
	}

/*  float *sTranspose(Matrix34A& m)
  {
    static Matrix44A dst;
    dst.m00=m.m00;	dst.m01=m.m10;	dst.m02=m.m20;	dst.m03=0;
    dst.m10=m.m01;	dst.m11=m.m11;	dst.m12=m.m21;	dst.m13=0;
    dst.m20=m.m02;	dst.m21=m.m12;	dst.m22=m.m22;	dst.m23=0;
    dst.m30=m.m03;	dst.m31=m.m13;	dst.m32=m.m23;	dst.m33=1;

    return dst.GetData();
  }
  void sTranspose(Matrix44A& m, Matrix44A *dst)
  {
    dst->m00=m.m00;	dst->m01=m.m10;	dst->m02=m.m20;	dst->m03=m.m30;
    dst->m10=m.m01;	dst->m11=m.m11;	dst->m12=m.m21;	dst->m13=m.m31;
    dst->m20=m.m02;	dst->m21=m.m12;	dst->m22=m.m22;	dst->m23=m.m32;
    dst->m30=m.m03;	dst->m31=m.m13;	dst->m32=m.m23;	dst->m33=m.m33;
  }
*/
  void sGetMotionBlurData(CD3D9Renderer *r)
  {
    // special motion blur instanced data:
    //  - contains (previous frame camera view) * (previous frame object matrix) 
    //  - merged transformation with object motion blur amount and shutter speed

    static int nLastObjFrame=-1;    
    static Matrix44A mCamObjCurr;
    
    static int nLastFrameID=-1;    
    static Vec3 pCamPrevPos = Vec3(0,0,0);
		const int nThreadID = r->m_RP.m_nProcessThreadID;

		Matrix44A &CamPrevMat = r->m_CameraMatrixPrev[ min(1, SRendItem::m_RenderView[nThreadID]) ];

    if( nLastFrameID != r->GetFrameID(true) )
    {
      // invert and get camera previous world space position
      Matrix44A CamPrevMatInv = CamPrevMat.GetInverted();
      pCamPrevPos = CamPrevMatInv.GetRow(3);

      nLastFrameID = gRenDev->GetFrameID(true);
    }

    if ( r->m_RP.m_FrameObject != nLastObjFrame )
    {
      float fMotionBlurAmount( 0.8f );
      nLastObjFrame = r->m_RP.m_FrameObject;

      CRenderObject *pObj = r->m_RP.m_pCurObject;

      Matrix44A mObjCurr;
      mObjCurr.Transpose(pObj->m_II.m_Matrix);   
      Matrix44A mObjPrev;
      SRenderObjData *pOD = pObj->GetObjData(nThreadID);
      mObjPrev.Transpose(pOD->m_prevMatrix);   
      assert (pOD);

      Matrix44A mCamObjPrev;
      
#ifdef ALLOW_CAMERA_SPACE
      if (r->m_RP.m_ObjFlags & FOB_CAMERA_SPACE)
      {
        Vec3 pCamCurrPos = r->GetRCamera().Orig;

        // set correct translation
        Vec3 pTranslation = pObj->m_II.m_Matrix.GetTranslation();
        pTranslation += pCamCurrPos;
        mObjCurr.SetRow(3, pTranslation );

        if (pOD)
          pTranslation = pOD->m_prevMatrix.GetTranslation();
        else
          pTranslation = pObj->m_II.m_Matrix.GetTranslation();
        pTranslation += pCamPrevPos;
        mObjPrev.SetRow(3, pTranslation );
      }
#endif
      mCamObjCurr.Multiply(mObjCurr, r->m_CameraMatrix); 
      mCamObjPrev.Multiply(mObjPrev, CamPrevMat); 
      float fExposureTime = CRenderer::CV_r_motionblur_shutterspeed * fMotionBlurAmount;

      // mk fix for time normalization

      // renormalize frametime to account for time scaling
      float fCurrFrameTime = gEnv->pTimer->GetFrameTime();      
      float fTimeScale = gEnv->pTimer->GetTimeScale();
      if (fTimeScale < 1.0f)
      {
        fTimeScale = max(0.0001f, fTimeScale);
        fCurrFrameTime /= fTimeScale; 
      }

      float fAlpha = 0.0f;
			if(fCurrFrameTime != 0.0f)
				fAlpha = fExposureTime / fCurrFrameTime;  

      if( CRenderer::CV_r_motionblurframetimescale )
      {
        float fAlphaScale = iszero(fCurrFrameTime) ? 1.0f : min(1.0f, (1.0f / fCurrFrameTime) / ( 32.0f)); // attenuate motion blur for lower frame rates
        fAlpha *= fAlphaScale;
      }

      mCamObjCurr =mCamObjCurr * (1.0f - fAlpha) + mCamObjPrev * fAlpha; 
      mCamObjCurr.Transpose();
    }

    float *pData = mCamObjCurr.GetData();
    // todo: use SSE
    sData[0].f[0] = pData[0]; sData[0].f[1] = pData[1]; sData[0].f[2] = pData[2]; sData[0].f[3] = pData[3];
    sData[1].f[0] = pData[4]; sData[1].f[1] = pData[5]; sData[1].f[2] = pData[6]; sData[1].f[3] = pData[7];
    sData[2].f[0] = pData[8]; sData[2].f[1] = pData[9]; sData[2].f[2] = pData[10]; sData[2].f[3] = pData[11];
  }

  void sGetCloakParams(CD3D9Renderer *r)
  {
		const int nThreadID = r->m_RP.m_nProcessThreadID;
    CRenderObject *pObj = r->m_RP.m_pCurObject;
    static int nLastObjFrame=-1;    
    static int nLastFrameID=-1;    
    static Vec4 pCloakParams;
    static Matrix44A mCamObjCurr;    
    static Vec3 pCamPrevPos = Vec3(0,0,0);

    if( nLastFrameID != gRenDev->GetFrameID(true) )
    {
      // invert and get camera previous world space position
      Matrix44A CamPrevMatInv = r->m_CameraMatrixPrev[ min(1, SRendItem::m_RenderView[nThreadID]) ].GetInverted();
      pCamPrevPos = CamPrevMatInv.GetRow(3);
      nLastFrameID = gRenDev->GetFrameID(true);
    }

    if ( r->m_RP.m_FrameObject != nLastObjFrame && pObj)
    {
      nLastObjFrame = r->m_RP.m_FrameObject;

      Vec3 pObjPosWS = pObj->m_II.m_Matrix.GetColumn(3);

      // Get amount of light on cpu - dont want to add more shader permutations - this might be useful for other stuff - expose
      float fLightAmount = 0.0f;
      for( uint32 nCurrDLight = 0; nCurrDLight < r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID]-1].Num() ; ++nCurrDLight )
      {
        if ( pObj->m_DynLMMask[r->m_RP.m_nProcessThreadID] & (1<<nCurrDLight) )
        {
          CDLight *pDL = &r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID]-1][nCurrDLight];

          // compute attenuation if not sun
          float fAttenuation = 1.0f;
          if( !(pDL->m_Flags & DLF_SUN) )
          {
            float fInvRadius = pDL->m_fRadius;
            if (fInvRadius <= 0)
              fInvRadius = 1.f;

            fInvRadius = 1.f / fInvRadius;

            // light position
            Vec3 pLightVec = pDL->m_Origin - pObjPosWS;

            // compute attenuation
            pLightVec *= fInvRadius;
            fAttenuation = clamp_tpl<float>(1.0f - (pLightVec.x * pLightVec.x + pLightVec.y * pLightVec.y + pLightVec.z * pLightVec.z), 0.0f, 1.0f);
          }

          fLightAmount += fAttenuation * ( (pDL->m_Color[0] + pDL->m_Color[1] + pDL->m_Color[2]) *0.33f );
        }
      }

      // Add ambient
      ColorF &pAmbient = r->m_RP.m_pCurInstanceInfo->m_AmbColor;
      fLightAmount += (pAmbient[0] + pAmbient[1] + pAmbient[2])*0.33f;

      // trying to match luminance bettwen hdr/non-hdr
      if( !CRenderer::CV_r_HDRRendering )
      {
        fLightAmount = 1.0f - expf( - fLightAmount );
        fLightAmount *= 2.5f;
      }
      else
        fLightAmount *= 0.25f;

        
      // Get cloak blend amount from material layers
      float fCloakBlendAmount = ((pObj->m_nMaterialLayers&0x0000ff00)>>8)/255.0f;

      // Get instance speed 
      float fMotionBlurAmount( 0.3f ); 
      nLastObjFrame = r->m_RP.m_FrameObject;

      CRenderObject *pObject = r->m_RP.m_pCurObject;

      Matrix44A mObjCurr;
      mObjCurr.Transpose(pObject->m_II.m_Matrix);   
      Matrix44A mObjPrev;
      SRenderObjData *pOD = pObject->GetObjData(r->m_RP.m_nProcessThreadID);
      assert(pOD);
      if (pOD)
        mObjPrev.Transpose(pOD->m_prevMatrix);   

      Matrix44A mCamObjPrev;
      float fSpeedScale = 1.0f;
#ifdef ALLOW_CAMERA_SPACE
      if (r->m_RP.m_ObjFlags & FOB_CAMERA_SPACE)
      {
        Vec3 pCamCurrPos = r->GetRCamera().Orig;

        // set correct translation
        Vec3 pTranslation = pObject->m_II.m_Matrix.GetTranslation();
        pTranslation += pCamCurrPos;
        mObjCurr.SetRow(3, pTranslation );

        pTranslation = pOD->m_prevMatrix.GetTranslation();
        pTranslation += pCamPrevPos;
        mObjPrev.SetRow(3, pTranslation );

        // make it less visible in first person
        fSpeedScale *= 0.25f;
      }
#endif
      mCamObjCurr = mObjCurr;
      mCamObjPrev = mObjPrev;

      // temporary solution for GC demo
      float fExposureTime = 0.0005f; //CRenderer::CV_r_motionblur_shutterspeed * fMotionBlurAmount; 

      // renormalize frametime to account for time scaling
      float fCurrFrameTime = gEnv->pTimer->GetFrameTime();      
      float fTimeScale = gEnv->pTimer->GetTimeScale();
      if (fTimeScale < 1.0f)
      {
        fTimeScale = max(0.0001f, fTimeScale);
        fCurrFrameTime /= fTimeScale; 
      }

      float fAlpha = 0.0f;
      if(fCurrFrameTime != 0.0f)
        fAlpha = fExposureTime / fCurrFrameTime;  
      
      mCamObjPrev = mCamObjCurr * (1.0f - fAlpha) + mCamObjPrev * fAlpha;  

      Vec3 pVelocity = mCamObjCurr.GetRow(3) - mCamObjPrev.GetRow(3);
      float fCurrSpeed = max( pVelocity.GetLength() - 0.01f, 0.0f);

      pCloakParams.x = pCloakParams.y = fCurrSpeed;
      pCloakParams.z = fLightAmount * r->CV_r_cloak_light_scale;
      pCloakParams.w = fCloakBlendAmount;
    }

    sData[0].f[0] = pCloakParams.x;
    sData[0].f[1] = pCloakParams.y;
    sData[0].f[2] = pCloakParams.z;
    sData[0].f[3] = pCloakParams.w;
  }

  void sGetFrozenParams(CD3D9Renderer *r)
  {
    CRenderObject *pObj = r->m_RP.m_pCurObject;
    static int nLastObjFrame=-1;    
    static Vec4 pFrozenParams;

    if ( r->m_RP.m_FrameObject != nLastObjFrame  && pObj)
    {
      nLastObjFrame = r->m_RP.m_FrameObject;

      // Get frost blend amount from material layers
      float fFrostBlendAmount = ((pObj->m_nMaterialLayers&0x000000ff))/255.0f;
      if( r->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_NEAREST] )
        fFrostBlendAmount *= 0.5f;

      float fUseObjSpace = (pObj->m_ObjFlags & FOB_MTLLAYERS_OBJSPACE)? 1.0f : 0.0f;

      if (r->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_SAMPLE4])
      {
        Vec3 pObjPosWS = pObj->m_II.m_Matrix.GetColumn(3);

        // Get amount of light on cpu - dont want to add more shader permutations - this might be useful for other stuff - expose
        ColorF pLightAmount = ColorF(0.0f, 0.0f, 0.0f, 0.0f);
        for( uint32 nCurrDLight = 0; nCurrDLight < r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID]-1].Num() ; ++nCurrDLight )
        {
          if ( pObj->m_DynLMMask[r->m_RP.m_nProcessThreadID] & (1<<nCurrDLight) )
          {
            CDLight *pDL = &r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID]-1][nCurrDLight];

            // compute attenuation if not sun
            float fAttenuation = 1.0f;
            if( !(pDL->m_Flags & DLF_SUN) )
            {
              float fInvRadius = pDL->m_fRadius;
              if (fInvRadius <= 0)
                fInvRadius = 1.f;

              fInvRadius = 1.f / fInvRadius;

              // light position
              Vec3 pLightVec = pDL->m_Origin - pObjPosWS;

              // compute attenuation	
              pLightVec *= fInvRadius;
              fAttenuation = clamp_tpl<float>(1.0f - (pLightVec.x * pLightVec.x + pLightVec.y * pLightVec.y + pLightVec.z * pLightVec.z), 0.0f, 1.0f);
            }

            pLightAmount += fAttenuation * pDL->m_Color;
          }
        }

        // Add ambient
        ColorF &pAmbient = r->m_RP.m_pCurInstanceInfo->m_AmbColor;
        //pLightAmount += pAmbient;

        pFrozenParams.x = pLightAmount[0];
        pFrozenParams.y = pLightAmount[1];
        pFrozenParams.z = pLightAmount[2];
        pFrozenParams.w = fFrostBlendAmount;
      }
      else
      {
        pFrozenParams.x = pFrozenParams.y = pFrozenParams.z = fFrostBlendAmount;
        pFrozenParams.w = fUseObjSpace;
      }
    }

    sData[0].f[0] = pFrozenParams.x;
    sData[0].f[1] = pFrozenParams.y;
    sData[0].f[2] = pFrozenParams.z;
    sData[0].f[3] = pFrozenParams.w;
  }
  NO_INLINE void sCausticsSmoothSunDirection()
  {
    SCGParamsPF &PF = gRenDev->m_cEF.m_PF;
    Vec3 v(0.0f,0.0f,0.0f);
    I3DEngine *pEng = gEnv->p3DEngine;  

    // Caustics are done with projection from sun - ence they update too fast with regular
    // sun direction. Use a smooth sun direction update instead to workaround this
    if( PF.nCausticsFrameID != gRenDev->GetFrameID(false) )
    {
      PF.nCausticsFrameID = gRenDev->GetFrameID(false);
			Vec3 pRealtimeSunDirNormalized = pEng->GetRealtimeSunDirNormalized();

			const float fSnapDot = 0.98f; 
      float fDot = fabs(PF.vCausticsCurrSunDir.Dot(pRealtimeSunDirNormalized));
      if( fDot < fSnapDot )
        PF.vCausticsCurrSunDir = pRealtimeSunDirNormalized;   

      PF.vCausticsCurrSunDir += (pRealtimeSunDirNormalized - PF.vCausticsCurrSunDir) * 0.005f * gEnv->pTimer->GetFrameTime();
      PF.vCausticsCurrSunDir.Normalize(); 
    }

    v = PF.vCausticsCurrSunDir;

    sData[0].f[0] = v.x;
    sData[0].f[1] = v.y;
    sData[0].f[2] = v.z;
    sData[0].f[3] = 0;
  }

  void sHMAGradients()
  {
    SRenderPipeline& RESTRICT_REFERENCE rRP = gRenDev->m_RP;
    CRendElementBase *pRE = rRP.m_pRE;
    assert(pRE->mfGetType() == eDATA_Mesh);
    CRenderMesh2* pRM = ((CREMesh *)pRE)->m_pRenderMesh;
    if (pRM)
    {
      AABB Box;
      pRM->GetBBox(Box.min,Box.max);
      CRenderObject *pObj = rRP.m_pCurObject;

      SRenderObjData *pOD = pObj->GetObjData(rRP.m_nProcessThreadID);
      assert(pOD);
      if (!pOD)
        return;

      const float RANGE	=	static_cast<float>((1<<2)-1)+0.5f;//0.5for rounding
      const unsigned int HMAIndex	=	pOD->m_HMAData;
      const float HMARange				=	*reinterpret_cast<float*>(&pOD->m_HMAData)/RANGE;

      const float H0	=	static_cast<float>(HMAIndex&((1<<3)-1))*HMARange-RANGE*HMARange;
      const float H1	=	static_cast<float>((HMAIndex>>3)&((1<<3)-1))*HMARange-RANGE*HMARange;
      const float H2	=	static_cast<float>((HMAIndex>>6)&((1<<3)-1))*HMARange-RANGE*HMARange;
      const float H3	=	static_cast<float>((HMAIndex>>9)&((1<<3)-1))*HMARange-RANGE*HMARange;

      const float DeltaX	=	Box.max.x-Box.min.x;
      const float DeltaY	=	Box.max.y-Box.min.y;
      if(fabs(DeltaX)>FLT_EPSILON && fabs(DeltaY)>FLT_EPSILON)
      {
        sData[0].f[0] = (H0-H1)/DeltaX;
        sData[0].f[1] = (H2-H3)/DeltaY;
        sData[0].f[2] = ((H0+H1)*0.5f)/(DeltaX*DeltaX*0.25f);
        sData[0].f[3] = ((H2+H3)*0.5f)/(DeltaY*DeltaY*0.25f);
      }
      else
      {
        sData[0].f[0]=sData[0].f[1]=sData[0].f[2]=sData[0].f[3]=0.f;
      }
    }
  }

  NO_INLINE void sAlphaTest()
  {
    SRenderPipeline& RESTRICT_REFERENCE rRP = gRenDev->m_RP;    
    sData[0].f[0] = (1.f/255.f)*rRP.m_pCurObject->m_DissolveRef;
    sData[0].f[1] = 0;
    sData[0].f[2] = CRenderer::CV_r_useSRGB != 0; // only way of doing test without adding more permutations
    sData[0].f[3] = rRP.m_pShaderResources ? rRP.m_pShaderResources->m_AlphaRef : 0;

    // specific condition for hair zpass
    if ((rRP.m_pShader->m_Flags2 & EF2_HAIR) && !(rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
      sData[0].f[3] = 0.51f;
  }

  NO_INLINE void sFurParams()
  {
    if (gRenDev->m_RP.m_pShaderResources)
    {
      sData[0].f[0] = gRenDev->m_RP.m_pShaderResources->FurAmount(); 
      sData[0].f[1] = sData[0].f[0];
      sData[0].f[2] = sData[0].f[0];
      sData[0].f[3] = sData[0].f[0];
    }
    else
      sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = sData[0].f[3] = 0.f;
  }
  NO_INLINE void sVisionParams()
  {
    CRenderObject *pObj = gRenDev->m_RP.m_pCurObject;
		float fRecip = (1.0f / 255.0f);
    sData[0].f[0] = float((pObj->m_nVisionParams&0xff000000)>>24) * fRecip;
    sData[0].f[1] = float((pObj->m_nVisionParams&0x00ff0000)>>16) * fRecip;
    sData[0].f[2] = float((pObj->m_nVisionParams&0x0000ff00)>>8 ) * fRecip;
		sData[0].f[3] = float((pObj->m_nVisionParams&0x000000ff) ) * fRecip;
    
		if( CRenderer::CV_r_customvisions == 2 )
			sData[0].f[3] = gEnv->pTimer->GetCurrTime() + ((float)(2 * pObj->m_Id) / 32768.0f );
  }

	NO_INLINE void sEffectLayerParams()
	{
		CRenderObject *pObj = gRenDev->m_RP.m_pCurObject;
		if( !CRenderer::CV_r_DebugLayerEffect )
		{
			SRenderObjData *pOD = pObj->GetObjData( gRenDev->m_RP.m_nProcessThreadID );
			if( pOD )
			{
				float fRecip = (1.0f / 255.0f);
				sData[0].f[0] = float((pOD->m_pLayerEffectParams[0]&0xff000000)>>24) * fRecip;
				sData[0].f[1] = float((pOD->m_pLayerEffectParams[0]&0x00ff0000)>>16) * fRecip;
				sData[0].f[2] = float((pOD->m_pLayerEffectParams[0]&0x0000ff00)>>8 ) * fRecip;
				sData[0].f[3] = float((pOD->m_pLayerEffectParams[0]&0x000000ff) ) * fRecip;
			}
			else
				sZeroLine();
		}
		else
		{
			
			const uint32 nDebugModesCount = 4;
			static const Vec4 pDebugModes[ nDebugModesCount ]=
			{
				Vec4( 1, 0, 0, 0),
				Vec4( 0, 1, 0, 0),
				Vec4( 0, 0, 1, 0),
				Vec4( 0, 0, 0, 1),
			};
			
			uint32 nCurrDebugMode = min( nDebugModesCount, CRenderer::CV_r_DebugLayerEffect) - 1;

			sData[0].f[0] = pDebugModes[ nCurrDebugMode ].x;
			sData[0].f[1] = pDebugModes[ nCurrDebugMode ].y;
			sData[0].f[2] = pDebugModes[ nCurrDebugMode ].z;
			sData[0].f[3] = pDebugModes[ nCurrDebugMode ].w;
		}

	}


  NO_INLINE void sMaterialLayersParams()
  {
    CRenderObject *pObj = gRenDev->m_RP.m_pCurObject;
    sData[0].f[0] = ((pObj->m_nMaterialLayers&0x000000ff))/255.0f;
    sData[0].f[1] = ((pObj->m_nMaterialLayers&0x00ff0000)>>16)/255.0f;
    // Apply attenuation
    sData[0].f[1] *= 1.0f - min(1.0f, pObj->m_fDistance / gRenDev->CV_r_rain_maxviewdist);

    sData[0].f[2] = ((pObj->m_nMaterialLayers&0x0000ff00)>>8)/255.0f;

    sData[0].f[3] = (pObj->m_ObjFlags & FOB_MTLLAYERS_OBJSPACE)? 1.0f : 0.0f;
  }

  NO_INLINE void sLightningColSize()
  {
    Vec3 v;
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, v);
    sData[0].f[0] = v.x;
    sData[0].f[1] = v.y;
    sData[0].f[2] = v.z;

    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_SIZE, v);
    sData[0].f[3] = v.x * 0.01f;			
  }

  NO_INLINE void sTexelsPerMeterInfo()
  {
    sData[0].f[0] = sData[0].f[1] = sData[0].f[2] = sData[0].f[3] = 0;
    SRenderShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;
    if (pRes && pRes->m_Textures[EFTT_DIFFUSE])
    {
      CTexture* pTexture(pRes->m_Textures[EFTT_DIFFUSE]->m_Sampler.m_pTex);
      if (pTexture)
      {
        int texWidth(pTexture->GetWidth());
        int texHeight(pTexture->GetHeight());
        float ratio = 0.5f / gRenDev->CV_r_TexelsPerMeter;
        sData[0].f[0] = (float) texWidth * ratio;
        sData[0].f[1] = (float) texHeight * ratio;
      }
    }
  }

  NO_INLINE void sOceanMat()
  {
    const CRenderCamera& cam(gRenDev->GetRCamera());

    Matrix44A viewMat;				
    viewMat.m00 = cam.X.x; viewMat.m01 = cam.Y.x; viewMat.m02 = cam.Z.x; viewMat.m03 = 0;					
    viewMat.m10 = cam.X.y; viewMat.m11 = cam.Y.y; viewMat.m12 = cam.Z.y; viewMat.m13 = 0;
    viewMat.m20 = cam.X.z; viewMat.m21 = cam.Y.z; viewMat.m22 = cam.Z.z; viewMat.m23 = 0;
    viewMat.m30 = 0; viewMat.m31 = 0; viewMat.m32 = 0; viewMat.m33 = 1;

    ((Matrix44A *)&sData[0])->Multiply(viewMat, *gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_matProj->GetTop());
    ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[0]);
  }

  NO_INLINE void sResInfoDiffuse()
  {
    sIdentityLine();
    SRenderShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;
    if (pRes && pRes->m_Textures[EFTT_DIFFUSE])
    {
      ITexture* pTexture(pRes->m_Textures[EFTT_DIFFUSE]->m_Sampler.m_pTex);
      if (pTexture)
      {
        int texWidth(pTexture->GetWidth());
        int texHeight(pTexture->GetHeight());
        sData[0].f[0] = (float) texWidth;
        sData[0].f[1] = (float) texHeight;
        sData[0].f[2] = 1.0f / (float) texWidth;
        sData[0].f[3] = 1.0f / (float) texHeight;
      }
    }
  }

  NO_INLINE void sTexelDensityParam()
  {
    sIdentityLine();

		SRenderShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;

		if (pRes && pRes->m_Textures[EFTT_DIFFUSE])
		{
			CRenderChunk *pRenderChunk = NULL;
			int texWidth = 512;
			int texHeight = 512;
			int mipLevel = 0;

			CRendElementBase *pRE = gRenDev->m_RP.m_pRE;

			if (pRE)
			{
				pRenderChunk = pRE->mfGetMatInfo();
			}

			CRenderObject *pCurObject = gRenDev->m_RP.m_pCurObject;

			if (pRenderChunk && pCurObject)
			{
				float weight = 1.0f;

				if (pRenderChunk->m_texelAreaDensity > 0.0f)
				{
					float scale = 1.0f;

					IRenderNode *pRenderNode = (IRenderNode *)pCurObject->m_pRenderNode;

					if (pRenderNode && pRenderNode != (void *)0xffffffff && pRenderNode != (void *)-1 && pRenderNode->GetRenderNodeType() == eERType_Brush)
					{
						scale = ((IBrush *)pRenderNode)->GetMatrix().GetColumn0().GetLength();
					}

					float distance = pCurObject->m_fDistance * TANGENT30_2 / scale;
					int screenHeight = gRenDev->GetHeight();

					weight = pRenderChunk->m_texelAreaDensity * distance * distance * texWidth * texHeight * pRes->m_Textures[EFTT_DIFFUSE]->m_TexModificator.m_Tiling[0] * pRes->m_Textures[EFTT_DIFFUSE]->m_TexModificator.m_Tiling[1] / (screenHeight * screenHeight);
				}

				mipLevel = fastround_positive(0.5f * logf(max(weight,1.0f)) / LN2);
			}

			texWidth /= (1 << mipLevel);
			texHeight /= (1 << mipLevel);

			if (texWidth == 0)
				texWidth = 1;
			if (texHeight == 0)
				texHeight = 1;

			sData[0].f[0] = (float) texWidth;
			sData[0].f[1] = (float) texHeight;
			sData[0].f[2] = 1.0f / (float) texWidth;
			sData[0].f[3] = 1.0f / (float) texHeight;
		}
  }

  NO_INLINE void sTexelDensityColor()
  {
    sOneLine();

		SRenderShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;

		if (pRes && pRes->m_Textures[EFTT_DIFFUSE])
		{
			if (gcpRendD3D->CV_e_DebugTexelDensity == 2 || gcpRendD3D->CV_e_DebugTexelDensity == 4)
			{
				CRenderChunk *pRenderChunk = NULL;
				int texWidth = 512;
				int texHeight = 512;
				int mipLevel = 0;

				CRendElementBase *pRE = gRenDev->m_RP.m_pRE;

				if (pRE)
				{
					pRenderChunk = pRE->mfGetMatInfo();
				}

				CRenderObject *pCurObject = gRenDev->m_RP.m_pCurObject;

				if (pRenderChunk && pCurObject)
				{
					float weight = 1.0f;

					if (pRenderChunk->m_texelAreaDensity > 0.0f)
					{
						float scale = 1.0f;

						IRenderNode *pRenderNode = (IRenderNode *)pCurObject->m_pRenderNode;

						if (pRenderNode && pRenderNode != (void *)0xffffffff && pRenderNode != (void *)-1 && pRenderNode->GetRenderNodeType() == eERType_Brush)
						{
							scale = ((IBrush *)pRenderNode)->GetMatrix().GetColumn0().GetLength();
						}

						float distance = pCurObject->m_fDistance * TANGENT30_2 / scale;
						int screenHeight = gRenDev->GetHeight();

						weight = pRenderChunk->m_texelAreaDensity * distance * distance * texWidth * texHeight * pRes->m_Textures[EFTT_DIFFUSE]->m_TexModificator.m_Tiling[0] * pRes->m_Textures[EFTT_DIFFUSE]->m_TexModificator.m_Tiling[1] / (screenHeight * screenHeight);
					}

					mipLevel = fastround_positive(0.5f * logf(max(weight,1.0f)) / LN2);
				}

				switch (mipLevel)
				{
					case 0:
						sData[0].f[0] = 1.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 1.0f; break;
					case 1:
						sData[0].f[0] = 0.0f; sData[0].f[1] = 0.0f; sData[0].f[2] = 1.0f; break;
					case 2:
						sData[0].f[0] = 0.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 0.0f; break;
					case 3:
						sData[0].f[0] = 0.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 1.0f; break;
					case 4:
						sData[0].f[0] = 1.0f; sData[0].f[1] = 0.0f; sData[0].f[2] = 0.0f; break;
					case 5:
						sData[0].f[0] = 1.0f; sData[0].f[1] = 0.0f; sData[0].f[2] = 1.0f; break;
					default:
						sData[0].f[0] = 1.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 0.0f; break;
				}
			}
			else
			{
				sData[0].f[0] = 1.0f; sData[0].f[1] = 1.0f; sData[0].f[2] = 1.0f;
			}
		}
	}

  NO_INLINE void sResInfoBump()
  {
    //PS3workaround
    sIdentityLine();
#if !defined(PS3)
    SRenderShaderResources *pRes = gRenDev->m_RP.m_pShaderResources;
    if (pRes && pRes->m_Textures[EFTT_BUMP])
    {
      ITexture* pTexture(pRes->m_Textures[EFTT_BUMP]->m_Sampler.m_pTex);
      if (pTexture)
      {
        int texWidth(pTexture->GetWidth());
        int texHeight(pTexture->GetHeight());
        sData[0].f[0] = (float) texWidth;
        sData[0].f[1] = (float) texHeight;
        sData[0].f[2] = 1.0f / (float) texWidth;
        sData[0].f[3] = 1.0f / (float) texHeight;
      }
    }
#endif
  }

  NO_INLINE void sNumInstructions()
  {
    sData[0].f[0] = gRenDev->m_RP.m_NumShaderInstructions / gRenDev->CV_r_measureoverdrawscale / 256.0f;
  }

  NO_INLINE void sSkyColor()
  {
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SCGParamsPF &PF = r->m_cEF.m_PF;
    I3DEngine *pEng = gEnv->p3DEngine;
    Vec3 v = pEng->GetSkyColor();
    sData[0].f[0] = v.x;
    sData[0].f[1] = v.y;
    sData[0].f[2] = v.z;
    sData[0].f[3] = CRenderer::CV_r_useSRGB != 0; // only way of doing test without adding more permutations;

    if (r->CV_r_PostProcess && r->CV_r_nightvision)
    {
      // If nightvision active, brighten up ambient
      if (PF.bPE_NVActive)
      {
        sData[0].f[0] += 0.25f;//0.75f;
        sData[0].f[1] += 0.25f;//0.75f;
        sData[0].f[2] += 0.25f;//0.75f;  
      }
    }
  }

  NO_INLINE void sAmbient(SRenderPipeline& rRP)
  {
    sData[0].f[0] = rRP.m_pCurInstanceInfo->m_AmbColor[0];
    sData[0].f[1] = rRP.m_pCurInstanceInfo->m_AmbColor[1]; 
    sData[0].f[2] = rRP.m_pCurInstanceInfo->m_AmbColor[2];  
    sData[0].f[3] = rRP.m_pCurInstanceInfo->m_AmbColor[3];

    if (SRenderShaderResources *pRes=rRP.m_pShaderResources)
    {
      sData[0].f[0] += pRes->m_Constants[eHWSC_Pixel][PS_EMISSIVE_COL][0];
      sData[0].f[1] += pRes->m_Constants[eHWSC_Pixel][PS_EMISSIVE_COL][1];
      sData[0].f[2] += pRes->m_Constants[eHWSC_Pixel][PS_EMISSIVE_COL][2];
      if (pRes->m_ResFlags & MTL_FLAG_ADDITIVE)
      {
        sData[0].f[0] *= rRP.m_fCurOpacity;
        sData[0].f[1] *= rRP.m_fCurOpacity;
        sData[0].f[2] *= rRP.m_fCurOpacity;
      }
    }
  }
  NO_INLINE void sAmbientOpacity()
  {
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SCGParamsPF & RESTRICT_REFERENCE PF = r->m_cEF.m_PF;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    CRenderObject *const __restrict pObj = rRP.m_pCurObject;
		
		float op = rRP.m_fCurOpacity;
		float a0 = rRP.m_pCurInstanceInfo->m_AmbColor[0];
		float a1 = rRP.m_pCurInstanceInfo->m_AmbColor[1];
		float a2 = rRP.m_pCurInstanceInfo->m_AmbColor[2];
		float a3 = rRP.m_pCurInstanceInfo->m_AmbColor[3];
		float opal = op * pObj->m_fAlpha;
    float s0 = a0;
    float s1 = a1; 
    float s2 = a2;
    float s3 = opal;// object opacity  

    if (pObj->m_nMaterialLayers)
      s3 *= sGetMaterialLayersOpacity(r);

    if (SRenderShaderResources *pRes=rRP.m_pShaderResources)
    {
      if (rRP.m_nShaderQuality == eSQ_Low)
      {
				float c0 = pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][0];
				float c1 = pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][1];
				float c2 = pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][2];

        s0 *= c0;
        s1 *= c1;
        s2 *= c2;
      }
      s0 += pRes->m_Constants[eHWSC_Pixel][PS_EMISSIVE_COL][0];
      s1 += pRes->m_Constants[eHWSC_Pixel][PS_EMISSIVE_COL][1];
      s2 += pRes->m_Constants[eHWSC_Pixel][PS_EMISSIVE_COL][2];

      if (pRes->m_ResFlags & MTL_FLAG_ADDITIVE)
      {
        s0 *= rRP.m_fCurOpacity;
        s1 *= rRP.m_fCurOpacity;
        s2 *= rRP.m_fCurOpacity;
      }

      if (r->CV_r_PostProcess && r->CV_r_nightvision)
      {
        // If nightvision active, brighten up ambient
        //   float fOffset = (CRenderer::CV_r_HDRRendering) ? 1.25f : 0.25;
        if (PF.bPE_NVActive)
        { 
          s0 += 0.25f;//fOffset;
          s1 += 0.25f;//fOffset;
          s2 += 0.25f;//fOffset;  
        }
      }
    }
  
#if !defined(XENON) && !defined(PS3)
    if(pObj->m_ObjFlags&FOB_SELECTED)
    {
      if(((int)(gEnv->pTimer->GetCurrTime()*8.f))&1)
      {
        s0 += 0.3f;
        s1 += 0.3f;
        s2 += 0.3f;
      }
    }
#endif
    sData[0].f[0] = s0;
    sData[0].f[1] = s1;
    sData[0].f[2] = s2;
    sData[0].f[3] = s3;
  }

  NO_INLINE void sObjectAmbColComp()
  {
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SCGParamsPF &PF = r->m_cEF.m_PF;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    CRenderObject *pObj = rRP.m_pCurObject;
    sData[0].f[0] = rRP.m_pCurInstanceInfo->m_AmbColor[3];
    sData[0].f[1] = /*r->m_RP.m_pCurInstanceInfo->m_AmbColor[3] * */rRP.m_fCurOpacity * pObj->m_fAlpha;

    if (pObj->m_nMaterialLayers)
      sData[0].f[1] *= sGetMaterialLayersOpacity( r );

    sData[0].f[3] = (float)pObj->m_nRenderQuality / 65535.0f;
    sData[0].f[2] = 0.f;
  }

  NO_INLINE void sMatEmissiveColor(const SCGParam *ParamBind, EHWShaderClass eSH)
  {
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SCGParamsPF &PF = r->m_cEF.m_PF;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    if (SRenderShaderResources *pRes=r->m_RP.m_pShaderResources)
    {
  #ifdef DIRECT3D10
      assert(0);
  #else
      Vec4 *pConsts = (Vec4 *)&pRes->m_Constants[eSH][0];
      pConsts -= FIRST_REG_PM[eSH];
      int i = ParamBind->m_dwBind;
      sData[0].f[0] = pConsts[i][0];
      sData[0].f[1] = pConsts[i][1];
      sData[0].f[2] = pConsts[i][2];
      sData[0].f[3] = pConsts[i][3];

      if (pRes->m_ResFlags & MTL_FLAG_ADDITIVE)
      {
        sData[0].f[0] *= rRP.m_fCurOpacity;
        sData[0].f[1] *= rRP.m_fCurOpacity;
        sData[0].f[2] *= rRP.m_fCurOpacity;
      }

      if (r->CV_r_PostProcess && r->CV_r_nightvision)
      {
        // If nightvision active, brighten up ambient
        if (PF.bPE_NVActive)
        { 
          sData[0].f[0] += 0.25f;//fOffset;
          sData[0].f[1] += 0.25f;//fOffset;
          sData[0].f[2] += 0.25f;//fOffset;  
        }
      }
  #endif
    }
    else
    {
      assert(0);
    }
  }

  NO_INLINE void sTweakable(const SCGParam *ParamBind, EHWShaderClass eSH)
  {
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    if (SRenderShaderResources *pRes=rRP.m_pShaderResources)
    {
  # ifdef DIRECT3D10
      assert(0);
  # else
      if (eSH < eHWSC_Max)
      {
        Vec4 *pConsts = (Vec4 *)&pRes->m_Constants[eSH][0];
        assert(ParamBind->m_dwBind-FIRST_REG_PM[eSH] < rRP.m_pShaderResources->m_Constants[eSH].size());
        SRenderObjData *pD = rRP.m_pCurObject->GetObjData(rRP.m_nProcessThreadID);
        if (pD && pD->m_Constants.size())
          pConsts = &pD->m_Constants[0];
        pConsts -= FIRST_REG_PM[eSH];
        int i = ParamBind->m_dwBind;
        sData[0].f[0] = pConsts[i][0];
        sData[0].f[1] = pConsts[i][1];
        sData[0].f[2] = pConsts[i][2];
        sData[0].f[3] = pConsts[i][3];

        if (pRes->m_ResFlags & MTL_FLAG_ADDITIVE)
        {
          sData[0].f[0] *= rRP.m_fCurOpacity;
          sData[0].f[1] *= rRP.m_fCurOpacity;
          sData[0].f[2] *= rRP.m_fCurOpacity;
        }
      }
  # endif
    }
    else
    {
      assert(0);
    }
  }

  NO_INLINE void sTextureTileSize(SRenderPipeline& rRP)
  {
    SRenderObjData *pOD = rRP.m_pCurObject->GetObjData(rRP.m_nProcessThreadID);
    if (pOD)
    {
      float* pTexTileSize = (float*)&pOD->m_fTempVars[0];
      sData[0].f[0] = pTexTileSize[0];
      sData[0].f[1] = pTexTileSize[1];
      sData[0].f[2] = pTexTileSize[2];
      sData[0].f[3] = pTexTileSize[3];
    }
  }

  NO_INLINE void sSunDirection()
  {
    I3DEngine *pEng = gEnv->p3DEngine;  
    Vec3 v = pEng->GetSunDirNormalized();
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    if((rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE) && (SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID]==2) && rRP.m_DLights[rRP.m_nProcessThreadID][1].Num())
    {
      v = rRP.m_DLights[rRP.m_nProcessThreadID][1][0].m_Origin;
      v.Normalize();
    }
    sData[0].f[0] = v.x;
    sData[0].f[1] = v.y;
    sData[0].f[2] = v.z;
    sData[0].f[3] = r->m_fAdaptedSceneScale;
  }

  NO_INLINE void sAvgFogVolumeContrib()
  {
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SCGParamsPF &PF = r->m_cEF.m_PF;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    CRenderObject *pObj = rRP.m_pCurObject;
    SRenderObjData *pOD = r->FX_GetObjData(pObj, true);
    assert(pOD);
    if (!pOD)
      return;

    bool bPush = false;
    if(pOD->m_FogVolumeContribIdx[rRP.m_nProcessThreadID] == (uint16) -1)
    {
      I3DEngine *pEng = gEnv->p3DEngine;
      ColorF newContrib;
      pEng->TraceFogVolumes(pObj->GetTranslation(), newContrib);

      pOD->m_FogVolumeContribIdx[rRP.m_nProcessThreadID] = r->PushFogVolumeContribution(newContrib);
      bPush = true;
    }

    const ColorF& contrib(r->GetFogVolumeContribution(pOD->m_FogVolumeContribIdx[rRP.m_nProcessThreadID]));
    // Pre-multiply alpha (saves 1 instruction in pixel shader)
    if (!rRP.m_bNotFirstPass)
    {
      sData[0].f[0] = contrib.r * (1 - contrib.a);   
      sData[0].f[1] = contrib.g * (1 - contrib.a);
      sData[0].f[2] = contrib.b * (1 - contrib.a);
    }
    else
    {
      sData[0].f[0] = 0;
      sData[0].f[1] = 0;
      sData[0].f[2] = 0;
    }

    sData[0].f[3] = contrib.a;
  }

  NO_INLINE void sDiffuseMulti()
  {
    CDLight *pDL;
    SLightPass *pLP;
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    pLP = &rRP.m_LPasses[rRP.m_nCurLightPass];
    for (uint32 i=0; i<pLP->nLights; i++)
    {
      pDL = pLP->pLights[i];
      sData[i].f[0] = pDL->m_Color[0];
      sData[i].f[1] = pDL->m_Color[1];
      sData[i].f[2] = pDL->m_Color[2];
      sData[i].f[3] = rRP.m_fCurOpacity * rRP.m_pCurObject->m_fAlpha * rRP.m_pCurInstanceInfo->m_AmbColor[3];

      if (rRP.m_pCurObject->m_nMaterialLayers)
        sData[i].f[3] *= sGetMaterialLayersOpacity(r);

      if (SRenderShaderResources *pRes=rRP.m_pShaderResources)
      {
        sData[i].f[0] *= pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][0];
        sData[i].f[1] *= pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][1];
        sData[i].f[2] *= pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][2];
        if (pRes->m_ResFlags & MTL_FLAG_ADDITIVE)
        {
          sData[i].f[0] *= rRP.m_fCurOpacity;
          sData[i].f[1] *= rRP.m_fCurOpacity;
          sData[i].f[2] *= rRP.m_fCurOpacity;
        }
      }
    }
  }

  NO_INLINE void sShadowMask()
  {
    CDLight *pDL;
    SLightPass *pLP;
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    pLP = &rRP.m_LPasses[rRP.m_nCurLightPass];
    for (uint32 i=0; i<pLP->nLights; i++)
    {
      pDL = pLP->pLights[i];
      sData[i].f[0] = pDL->m_ShadowChanMask.x;
      sData[i].f[1] = pDL->m_ShadowChanMask.y;
      sData[i].f[2] = pDL->m_ShadowChanMask.z;
      sData[i].f[3] = pDL->m_ShadowChanMask.w;
    }
  }

  NO_INLINE void sDepthFactor(CD3D9Renderer *r)
  {
    const CRenderCamera& rc = r->GetRCamera();
    float zn = rc.Near;
    float zf = rc.Far; 
    sData[0].f[3] = -(zf/(zf-zn));

    sData[0].f[0] = 255.0/256.0;
    sData[0].f[1] = 255.0/65536.0;
    sData[0].f[2] = 255.0/16777216.0; 
  }
  NO_INLINE void sNearFarDist(CD3D9Renderer *r)
  {
    const CRenderCamera& rc = r->GetRCamera();
    I3DEngine *pEng = gEnv->p3DEngine;
    sData[0].f[0] = rc.Near;
    sData[0].f[1] = rc.Far;
    // NOTE : v[2] is used to put the weapon's depth range into correct relation to the whole scene 
    // when generating the depth texture in the z pass (_RT_NEAREST) 
    sData[0].f[2] = rc.Far / pEng->GetMaxViewDistance(); 
    sData[0].f[3] = 1.0f / rc.Far;
  }

  NO_INLINE void sGetTempData(CD3D9Renderer *r, const SCGParam *ParamBind)
  {
    sData[0].f[0] = r->m_cEF.m_TempVecs[ParamBind->m_nID].x;
    sData[0].f[1] = r->m_cEF.m_TempVecs[ParamBind->m_nID].y;
    sData[0].f[2] = r->m_cEF.m_TempVecs[ParamBind->m_nID].z;
    sData[0].f[3] = r->m_cEF.m_TempVecs[ParamBind->m_nID].w;
  }

  NO_INLINE void sCameraFront(CD3D9Renderer *r)
  {
    Vec3 v = r->GetRCamera().Z;
    v.Normalize();

    sData[0].f[0] = v.x;
    sData[0].f[1] = v.y;
    sData[0].f[2] = v.z;
    sData[0].f[3] = 0;
  }
  NO_INLINE void sCameraRight(CD3D9Renderer *r)
  {
    Vec3 v = r->GetRCamera().X;
    v.Normalize();

    sData[0].f[0] = v.x;
    sData[0].f[1] = v.y;
    sData[0].f[2] = v.z;
    sData[0].f[3] = 0;
  }
  NO_INLINE void sCameraUp(CD3D9Renderer *r)
  {
    Vec3 v = r->GetRCamera().Y;
    v.Normalize();

    sData[0].f[0] = v.x;
    sData[0].f[1] = v.y;
    sData[0].f[2] = v.z;
    sData[0].f[3] = 0;
  }

  NO_INLINE void sRTRect(CD3D9Renderer *r)
  {
    sData[0].f[0] = r->m_cEF.m_RTRect.x;
    sData[0].f[1] = r->m_cEF.m_RTRect.y;
    sData[0].f[2] = r->m_cEF.m_RTRect.z;
    sData[0].f[3] = r->m_cEF.m_RTRect.w;
  }

#ifndef EXCLUDE_SCALEFORM_SDK
  NO_INLINE void sSFCompMat()
  {
    const SSF_GlobalDrawParams* pParams(gRenDev->SF_GetGlobalDrawParams());
    assert(pParams);
    if (pParams)
    {
      int viewportX0, viewportY0, viewportWidth, viewportHeight;
      gRenDev->GetViewport(&viewportX0, &viewportY0, &viewportWidth, &viewportHeight);

      Matrix44A& matComposite((Matrix44A&)sData[0].f[0]);
      Matrix44A matProj;
      mathMatrixOrthoOffCenterLH(&matProj, (f32) viewportX0, (f32) (viewportX0 + viewportWidth), (f32) (viewportY0 + viewportHeight), (f32) viewportY0, 0.0f, 1.0f);
      matProj.Transpose();
#if defined(DIRECT3D9)
      matProj.m03 -= 1.0f / (float) viewportWidth;
      matProj.m13 += 1.0f / (float) viewportHeight;
#endif
      Matrix44A matTrans(*pParams->pTransMat);
      matComposite = matProj * matTrans;
    }
  }

  NO_INLINE void sSFTexGenMat0(CD3D9Renderer *r)
  {
    const SSF_GlobalDrawParams* p(r->SF_GetGlobalDrawParams());
    assert(p);
    if (p)
    {
      const Matrix34A& mat(p->texture[0].texGenMat);
      sData[0].f[0] = mat.m00;
      sData[0].f[1] = mat.m01;
      sData[0].f[2] = mat.m02;
      sData[0].f[3] = mat.m03;

      sData[1].f[0] = mat.m10;
      sData[1].f[1] = mat.m11;
      sData[1].f[2] = mat.m12;
      sData[1].f[3] = mat.m13;
    }
  }
  NO_INLINE void sSFTexGenMat1(CD3D9Renderer *r)
  {
    const SSF_GlobalDrawParams* p(r->SF_GetGlobalDrawParams());
    assert(p);
    if (p)
    {
      const Matrix34A& mat(p->texture[1].texGenMat);
      sData[0].f[0] = mat.m00;
      sData[0].f[1] = mat.m01;
      sData[0].f[2] = mat.m02;
      sData[0].f[3] = mat.m03;

      sData[1].f[0] = mat.m10;
      sData[1].f[1] = mat.m11;
      sData[1].f[2] = mat.m12;
      sData[1].f[3] = mat.m13;
    }
  }
  NO_INLINE void sSFBitmapColorTransform(CD3D9Renderer *r)
  {
    const SSF_GlobalDrawParams* p(r->SF_GetGlobalDrawParams());
    assert(p);
    if (p)
    {
      const ColorF& col1st(p->colTransform1st);
      sData[0].f[0] = col1st.r;
      sData[0].f[1] = col1st.g;
      sData[0].f[2] = col1st.b;
      sData[0].f[3] = col1st.a;

      const ColorF& col2nd(p->colTransform2nd);
      sData[1].f[0] = col2nd.r;
      sData[1].f[1] = col2nd.g;
      sData[1].f[2] = col2nd.b;
      sData[1].f[3] = col2nd.a;
    }
  }
#endif

  NO_INLINE void sSkyLightSunDirection(CD3D9Renderer *r)
  {
    const SSkyLightRenderParams* p(r->GetSkyLightRenderParams());
    if (p)
    {
      sData[0].f[0] = p->m_sunDirection.x;
      sData[0].f[1] = p->m_sunDirection.y;
      sData[0].f[2] = p->m_sunDirection.z;
      sData[0].f[3] = 0;
    }
    else
    {
      //assert( !"Some shader refers to currently not available sky light constants!" );
      sZeroLine();
    }
  }
  NO_INLINE void sSkyLightPhaseFunctionConstants(CD3D9Renderer *r)
  {
    const SSkyLightRenderParams* p(r->GetSkyLightRenderParams());
    if (p)
    {
      sData[0].f[0] = p->m_phaseFunctionConsts.x;
      sData[0].f[1] = p->m_phaseFunctionConsts.y;
      sData[0].f[2] = p->m_phaseFunctionConsts.z;
      sData[0].f[3] = p->m_phaseFunctionConsts.w;
    }
    else
    {
      //assert( !"Some shader refers to currently not available sky light constants!" );
      sZeroLine();
    }
  }
  NO_INLINE void sSkyLightHazeColorPartialRayleighInScatter(SCGParamsPF& PF)
  {
    if (PF.pSkyLightRenderParams)
    {
      sData[0].f[0] = PF.pSkyLightHazeColorPartialRayleighInScatter.x;
      sData[0].f[1] = PF.pSkyLightHazeColorPartialRayleighInScatter.y;
      sData[0].f[2] = PF.pSkyLightHazeColorPartialRayleighInScatter.z;
      sData[0].f[3] = 0;
    }
    else
    {
      //assert( !"Some shader refers to currently not available sky light constants!" );
      sZeroLine();
    }
  }
  NO_INLINE void sSkyLightHazeColorPartialMieInScatter(SCGParamsPF& PF)
  {
    if (PF.pSkyLightRenderParams)
    {
      sData[0].f[0] = PF.pSkyLightHazeColorPartialMieInScatter.x;
      sData[0].f[1] = PF.pSkyLightHazeColorPartialMieInScatter.y;
      sData[0].f[2] = PF.pSkyLightHazeColorPartialMieInScatter.z;
      sData[0].f[3] = 0;
    }
    else
    {
      //assert( !"Some shader refers to currently not available sky light constants!" );
      sZeroLine();
    }
  }

  NO_INLINE void sSpecularMulti()
  {
    CDLight *pDL;
    SLightPass *pLP;
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    pLP = &rRP.m_LPasses[rRP.m_nCurLightPass];
    for (uint32 i=0; i<pLP->nLights; i++)
    {
      pDL = pLP->pLights[i];
      sData[i].f[0] = pDL->m_Color[0] * pDL->m_Color.a;
      sData[i].f[1] = pDL->m_Color[1] * pDL->m_Color.a;
      sData[i].f[2] = pDL->m_Color[2] * pDL->m_Color.a;
      sData[i].f[3] = pDL->m_SpecMult;

      if (rRP.m_pShaderResources)
      {
        sData[i].f[0] *= rRP.m_pShaderResources->m_Constants[eHWSC_Pixel][PS_SPECULAR_COL][0];
        sData[i].f[1] *= rRP.m_pShaderResources->m_Constants[eHWSC_Pixel][PS_SPECULAR_COL][1];
        sData[i].f[2] *= rRP.m_pShaderResources->m_Constants[eHWSC_Pixel][PS_SPECULAR_COL][2];
      }

      if (rRP.m_pShaderResources && (rRP.m_pShaderResources->m_ResFlags & MTL_FLAG_ADDITIVE))
      {
        sData[i].f[0] *= rRP.m_fCurOpacity;
        sData[i].f[1] *= rRP.m_fCurOpacity;
        sData[i].f[2] *= rRP.m_fCurOpacity;
      }
    }
  }

  NO_INLINE void sLightPos()
  {
    CDLight *pDL;
    SLightPass *pLP;
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    pLP = &rRP.m_LPasses[rRP.m_nCurLightPass];
    for (uint32 i=0; i<pLP->nLights; i++)
    {
      pDL = pLP->pLights[i];
      Vec3 v = pDL->m_Origin - r->GetRCamera().Orig;
      sData[i].f[0] = v.x;
      sData[i].f[1] = v.y;
      sData[i].f[2] = v.z;

      float fRadius = pDL->m_fRadius;
      if (fRadius <= 0)
        fRadius = 1.f;
      sData[i].f[3] = 1.f / fRadius;
    }
  }

  NO_INLINE void sLightsNum(int nComp)
  {
    SLightPass *pLP;
    CD3D9Renderer *const __restrict r = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
    pLP = &rRP.m_LPasses[rRP.m_nCurLightPass];
    sData[0].f[nComp] = (float)pLP->nLights;
  }


}

void CRenderer::UpdateConstParamsPF( )
{
  // Per frame - hardcoded/fast - update of commonly used data - feel free to improve this
  int nThreadID = m_RP.m_nFillThreadID;

  SCGParamsPF &PF = gRenDev->m_cEF.m_PF;
  uint32 nFrameID = gRenDev->m_RP.m_TI[nThreadID].m_nFrameUpdateID;
  if( PF.nFrameID == nFrameID || SRendItem::m_RecurseLevel[nThreadID] > 1 )
    return;

  PF.nFrameID = nFrameID;

  // Updating..

  I3DEngine *p3DEngine = gEnv->p3DEngine;
  if (p3DEngine==NULL)
    return;


  // ECGP_PB_WaterLevel - x = static level y = dynamic water ocean/volume level based on camera position, z: dynamic ocean water level
  PF.vWaterLevel = Vec3(p3DEngine->GetWaterLevel(), gEnv->p3DEngine->GetWaterLevel(&gRenDev->GetRCamera().Orig), p3DEngine->GetOceanWaterLevel(gRenDev->GetRCamera().Orig));

  // ECGP_PB_HDRDynamicMultiplier
  PF.fHDRDynamicMultiplier = p3DEngine->GetHDRDynamicMultiplier();

  // ECGP_PB_VolumetricFogParams
  PF.pVolumetricFogParams = sGetVolumetricFogParams( gcpRendD3D );
  // ECGP_PB_VolumetricFogRampParams
  PF.pVolumetricFogRampParams = sGetVolumetricFogRampParams();
  // ECGP_PB_VolumetricFogColor
  PF.pVolumetricFogColor = p3DEngine->GetFogColor();

  sGetFogColorGradientConstanst(PF.pFogColGradColBase, PF.pFogColGradColDelta);

  Vec4 vTmp;

  const SSkyLightRenderParams *pSkyParams = gRenDev->GetSkyLightRenderParams();
  if( pSkyParams )
  {
    PF.pSkyLightRenderParams = const_cast<SSkyLightRenderParams *>( pSkyParams );

    // ECGP_PB_SkyLightHazeColorPartialMieInScatter
    vTmp = pSkyParams->m_hazeColorMieNoPremul * pSkyParams->m_partialMieInScatteringConst;
    PF.pSkyLightHazeColorPartialMieInScatter = Vec3(vTmp.x, vTmp.y, vTmp.z);

    // ECGP_PB_SkyLightHazeColorPartialRayleighInScatter
    PF.pSkyLightHazeColorPartialRayleighInScatter = pSkyParams->m_hazeColorRayleighNoPremul * pSkyParams->m_partialRayleighInScatteringConst;
  }

  // ECGP_PB_CausticsParams
  vTmp = p3DEngine->GetCausticsParams();
  PF.pCausticsParams = Vec3( vTmp.y, vTmp.z, vTmp.w );

  // ECGP_PF_SunColor
  PF.pSunColor = p3DEngine->GetSunColor();
  // ECGP_PF_SkyColor
  PF.pSkyColor = p3DEngine->GetSkyColor();

  //ECGP_PB_CloudShadingColorSun
  float fSunColorMul, fSkyColorMul;
  p3DEngine->GetCloudShadingMultiplier(fSunColorMul, fSkyColorMul);
  PF.pCloudShadingColorSun = (fSunColorMul * PF.pSunColor); 
  //ECGP_PB_CloudShadingColorSky
  PF.pCloudShadingColorSky =  (fSkyColorMul * PF.pSkyColor);

  // ECGP_PB_DecalZFightingRemedy
  float *mProj = (float *)gcpRendD3D->m_RP.m_TI[nThreadID].m_matProj->GetTop();
  float s = clamp_tpl(CRenderer::CV_r_ZFightingDepthScale, 0.1f, 1.0f);

  PF.pDecalZFightingRemedy.x = s; // scaling factor to pull decal in front
  PF.pDecalZFightingRemedy.y = (float)((1.0f - s) * mProj[4*3+2]); // correction factor for homogeneous z after scaling is applied to xyzw { = ( 1 - v[0] ) * zMappingRageBias }
  PF.pDecalZFightingRemedy.z = clamp_tpl(CRenderer::CV_r_ZFightingExtrude, 0.0f, 1.0f);

  // alternative way the might save a bit precision
  //PF.pDecalZFightingRemedy.x = s; // scaling factor to pull decal in front
  //PF.pDecalZFightingRemedy.y = (float)((1.0f - s) * mProj[4*2+2]);
  //PF.pDecalZFightingRemedy.z = clamp_tpl(CRenderer::CV_r_ZFightingExtrude, 0.0f, 1.0f);

  using namespace NPostEffects;
  {
    static CEffectParam *pNVParam = PostEffectMgr().GetByName("NightVision_Active"); 
		if (pNVParam)
		{
			PF.bPE_NVActive = pNVParam->GetParam() != 0.0f;
		}
  }
}




























#if defined (DIRECT3D10) || defined(PS3)
std::vector<CHWShader_D3D::SCBuffer> CHWShader_D3D::m_CB_SI;
std::vector<ID3D11Buffer *> CHWShader_D3D::m_CB_SI_Released[CB_SI_MAXVECS];
ID3D11Buffer * CHWShader_D3D::m_CB_SI_Staged[CB_SI_MAXVECS];
std::vector<int> CHWShader_D3D::m_CB_SI_ReleasedID;

#ifdef _DEBUG
void CHWShader_D3D::mfValidateCB_SI(CRenderObject *pObj, CShader *pFXShader, int nMaxVecs)
{
  SCBuffer& cb = m_CB_SI[pObj->m_nCBID]; 
  Vec4 *pData = &cb.Vectors[0];
  Vec4 *vData = (Vec4 *)&sData[16];
  SCGParam *pParam = &pFXShader->m_InstParams[0];
  int nParams = pFXShader->m_nInstParams;
  mfSetParametersPI(pParam, nParams, &vData[0][0], eHWSC_Vertex, nMaxVecs);
  assert(pParam->m_dwBind == 0);  // First should be matrix
  assert (pData[0] == vData[0]);
  assert (pData[1] == vData[1]);
  assert (pData[2] == vData[2]);
  pParam++;
  for (int i=1; i<nParams; i++, pParam++)
  {
    if (pParam->m_dwCBufSlot < 0)
      continue;
    assert(pData[i+2] == vData[i+2]);
  }
}
#endif

void CHWShader::mfReleaseCB_SI(int nCBID)
{
  ID3D11Buffer *pBuf = CHWShader_D3D::mfGetCB_SI_Interface(nCBID);
  assert(pBuf);
  if (!pBuf)
    return;
  D3D11_BUFFER_DESC Desc;
  pBuf->GetDesc(&Desc);
  int nVecs = Desc.ByteWidth >> 4;
  assert(nVecs < CB_SI_MAXVECS);
  CHWShader_D3D::m_CB_SI_Released[nVecs].push_back(pBuf);
  CHWShader_D3D::m_CB_SI_ReleasedID.push_back(nCBID);
}

int CHWShader_D3D::mfGetCB_SI(int nMaxVecs, CShader *pFXShader)
{
  ID3D11Buffer *pBuf = NULL;
  assert(nMaxVecs < CB_SI_MAXVECS);
  if (m_CB_SI_Released[nMaxVecs].size())
  {
    pBuf = m_CB_SI_Released[nMaxVecs][m_CB_SI_Released[nMaxVecs].size()-1];
    m_CB_SI_Released[nMaxVecs].pop_back();
  }
  else
  {
    D3D11_BUFFER_DESC bd;
    ZeroStruct(bd);
    HRESULT hr;
    bd.MiscFlags = 0;
    bd.ByteWidth = nMaxVecs * sizeof(Vec4);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    //bd.Usage = D3D11_USAGE_DYNAMIC;
    //bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = gcpRendD3D->m_pd3dDevice->CreateBuffer(&bd, NULL, &pBuf);
  }
  assert(pBuf);
  SCBuffer cb;
  cb.nMask = pFXShader->m_nMaskCB;
  cb.pBuf = pBuf;
  int nID;
  if (m_CB_SI_ReleasedID.size())
  {
    nID = m_CB_SI_ReleasedID[m_CB_SI_ReleasedID.size()-1];
    m_CB_SI_ReleasedID.pop_back();
    assert(nID < m_CB_SI.size());
    m_CB_SI[nID] = cb;
  }
  else
  {
    nID = m_CB_SI.size();
    m_CB_SI.push_back(cb);
  }
  Vec4 *pData;
  /*pBuf->Map(D3D11_MAP_WRITE_DISCARD, NULL, (void **)&pData);
  Vec4 *vData = (Vec4 *)&sData[16];
  const int nParams = pFXShader->m_InstParams.size();
  SCGParam *pParam = &pFXShader->m_InstParams[0];
  mfSetParametersPI(pParam, nParams, &vData[0][0], eHWSC_Vertex, nMaxVecs);
  assert(pParam->m_dwBind == 0);  // First should be matrix
  pData[0] = vData[0];
  pData[1] = vData[1];
  pData[2] = vData[2];
  pParam++;
  for (int i=1; i<nParams; i++, pParam++)
  {
    int nBind = pParam->m_dwBind>>2;
    pData[nBind] = vData[i+2];
  }
  pBuf->Unmap();*/
  ID3D11Buffer *pTempBuf = m_CB_SI_Staged[nMaxVecs];
  if (!pTempBuf)
  {
    D3D11_BUFFER_DESC BufDesc;
    ZeroStruct(BufDesc);
    BufDesc.ByteWidth = nMaxVecs * sizeof(Vec4);
    BufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    BufDesc.Usage = D3D11_USAGE_STAGING;
    BufDesc.BindFlags = 0;
    BufDesc.MiscFlags = 0;
    HRESULT hr = gcpRendD3D->m_pd3dDevice->CreateBuffer(&BufDesc, NULL, &pTempBuf);
    m_CB_SI_Staged[nMaxVecs] = pTempBuf;
  }
  assert(pTempBuf);
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = gcpRendD3D->m_pd3dDeviceContext->Map(pTempBuf, 0, D3D11_MAP_WRITE, 0, &mappedResource);
	pData = (Vec4*)mappedResource.pData;
  Vec4 *vData = (Vec4 *)&sData[16];
  SCGParam *pParam = &pFXShader->m_InstParams[0];
  int nParams = pFXShader->m_nInstParams;
  mfSetParametersPI(pParam, nParams, &vData[0][0], eHWSC_Vertex, nMaxVecs);
  assert(pParam->m_dwBind == 0);  // First should be matrix
  pData[0] = vData[0];
  pData[1] = vData[1];
  pData[2] = vData[2];
  
#ifdef _DEBUG
  SCBuffer &c = m_CB_SI[nID];
  c.Vectors.push_back(vData[0]);
  c.Vectors.push_back(vData[1]);
  c.Vectors.push_back(vData[2]);
#endif
  pParam++;
  for (int i=1; i<nParams; i++, pParam++)
  {
    if (pParam->m_dwCBufSlot < 0)
      continue;
    int nBind = pParam->m_dwBind;
    pData[nBind] = vData[i+2];
#ifdef _DEBUG
    c.Vectors.push_back(vData[i+2]);
#endif
  }
	gcpRendD3D->m_pd3dDeviceContext->Unmap(pTempBuf, 0);
  gcpRendD3D->m_pd3dDeviceContext->CopyResource(pBuf, pTempBuf);

  return nID;
}
#endif

void CHWShader_D3D::mfCommitParams(bool bSetPM)
{
  CD3D9Renderer *const __restrict rd = gcpRendD3D;
#ifdef MERGE_SHADER_PARAMETERS
  SThreadInfo& RESTRICT_REFERENCE rTI = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID];
#if defined (DIRECT3D9)
  PROFILE_FRAME(CommitShaderParams);

  if (rTI.m_PersFlags2 & RBPF2_COMMIT_PF)
  {
    rTI.m_PersFlags2 &= ~RBPF2_COMMIT_PF;
    mfSetPF();
  }
  if (rTI.m_PersFlags2 & RBPF2_COMMIT_CM)
  {
    rTI.m_PersFlags2 &= ~RBPF2_COMMIT_CM;
    mfSetCM();
  }

#ifndef OPENGL //XENON
  LPDIRECT3DDEVICE9 dv = rd->GetD3DDevice();
  int i;
  if (m_NumPSParamsToCommit > 0)
  {
    //std::sort(m_PSParamsToCommit, m_PSParamsToCommit+m_NumPSParamsToCommit);

    int nFirst = m_PSParamsToCommit[0];
    int nParams = 1;
		assert(nFirst < MAX_CONSTANTS_PS);
    const int nCommitParms = m_NumPSParamsToCommit;
    for (i=1; i<nCommitParms; i++)
    {
      if (m_PSParamsToCommit[i] != m_PSParamsToCommit[i-1]+1)
      {
 //#if defined XENON
 //       sSetXPS(nFirst, nParams);
 //#else
        dv->SetPixelShaderConstantF(nFirst, &m_CurPSParams[nFirst].x, nParams);
 //#endif
        nFirst = m_PSParamsToCommit[i];
        nParams = 1;
      }
      else
        nParams++;
    }
 //#if defined XENON
 //   sSetXPS(nFirst, nParams);
 //#else
		assert(nFirst < MAX_CONSTANTS_PS);
    dv->SetPixelShaderConstantF(nFirst, &m_CurPSParams[nFirst].x, nParams);
 //#endif
    m_NumPSParamsToCommit = 0;
  }

  if (m_NumVSParamsToCommit > 0)
  {
    //std::sort(m_VSParamsToCommit, m_VSParamsToCommit+m_NumVSParamsToCommit);

    int nFirst = m_VSParamsToCommit[0];
    int nParams = 1;
		assert(nFirst < MAX_CONSTANTS_VS);
    const int nCommitParms = m_NumVSParamsToCommit;
    for (i=1; i<nCommitParms; i++)
    {
      if (m_VSParamsToCommit[i] != m_VSParamsToCommit[i-1]+1)
      {
 //#if defined XENON
 //       sSetXVS(nFirst, nParams);
 //#else
        dv->SetVertexShaderConstantF(nFirst, &m_CurVSParams[nFirst].x, nParams);
 //#endif
        nFirst = m_VSParamsToCommit[i];
        nParams = 1;
      }
      else
        nParams++;
    }
 //#if defined XENON
 //   sSetXVS(nFirst, nParams);
 //#else
		assert(nFirst < MAX_CONSTANTS_VS);
    dv->SetVertexShaderConstantF(nFirst, &m_CurVSParams[nFirst].x, nParams);
 //#endif
    m_NumVSParamsToCommit = 0;
  }
#endif

#elif defined(DIRECT3D10)
  if (rTI.m_PersFlags2 & (RBPF2_COMMIT_PF | RBPF2_COMMIT_CM))
  {
    rTI.m_PersFlags2 &= ~(RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);
    mfSetPF();
    mfSetCM();
  }








  for (int j=0; j<eHWSC_Max; j++)
  {
    for (int i=0; i<CB_MAX; i++)
    {
      mfCommitCB(i, (EHWShaderClass)j);
    }
  }

  if (bSetPM)
  {
    SRenderShaderResources *const __restrict pRes = rd->m_RP.m_pShaderResources;
    if (pRes)
    {
      if (m_pCurInstVS && m_pCurInstVS->m_bHasPMParams)
      {
        SRenderObjData *const __restrict pOD =  rd->m_RP.m_pCurObject->GetObjData(rd->m_RP.m_nProcessThreadID); //gRenDev->EF_GetObjData(rd->m_RP.m_pCurObject, false);
        if (pOD && pOD->m_Constants.size())
        {
          mfSetCBConst(0, CB_PER_MATERIAL, eHWSC_Vertex, &pOD->m_Constants[0][0], pOD->m_Constants.size()*4, pOD->m_Constants.size());
          mfCommitCB(CB_PER_MATERIAL, eHWSC_Vertex);
        }
        else
        {
          ID3D11Buffer *const __restrict pCB = (ID3D11Buffer *)pRes->m_pCB[eHWSC_Vertex];
          mfSetCB(eHWSC_Vertex, CB_PER_MATERIAL, pCB);
        }
      }
      if (m_pCurInstPS && m_pCurInstPS->m_bHasPMParams)
      {
        ID3D11Buffer *const __restrict pCB = (ID3D11Buffer *)pRes->m_pCB[eHWSC_Pixel];
        mfSetCB(eHWSC_Pixel, CB_PER_MATERIAL, pCB);
      }
      if (GEOMETRYSHADER_SUPPORT && m_pCurInstGS && m_pCurInstGS->m_bHasPMParams)
      {
        ID3D11Buffer *const __restrict pCB = (ID3D11Buffer *)pRes->m_pCB[eHWSC_Geometry];
        mfSetCB(eHWSC_Geometry, CB_PER_MATERIAL, pCB);
      }
    }
    if (m_pCurReqCB[eHWSC_Pixel][CB_PER_LIGHT])
      mfSetCB(eHWSC_Pixel, CB_PER_LIGHT, m_pCurReqCB[eHWSC_Pixel][CB_PER_LIGHT]);
    if (m_pCurReqCB[eHWSC_Vertex][CB_PER_LIGHT])
      mfSetCB(eHWSC_Vertex, CB_PER_LIGHT, m_pCurReqCB[eHWSC_Vertex][CB_PER_LIGHT]);
    if (GEOMETRYSHADER_SUPPORT && m_pCurReqCB[eHWSC_Geometry][CB_PER_LIGHT])
      mfSetCB(eHWSC_Geometry, CB_PER_LIGHT, m_pCurReqCB[eHWSC_Geometry][CB_PER_LIGHT]);
  }

#elif defined(OPENGL)
  // mfSetVSConst() and mfSetPSConst() will set the constant directly through
  // the IDirect3DDevice9/OpenGL wrapper.
  assert(m_NumPSParamsToCommit == 0);
  assert(m_NumVSParamsToCommit == 0);
#endif
#endif
}


static char *sSH[] = {"VS", "PS", "GS"};
static char *sComp[] = {"x", "y", "z", "w"};

#if defined (DIRECT3D10)
float *CHWShader_D3D::mfSetParametersPI(SCGParam *pParams, const int nINParams, float *pDst, EHWShaderClass eSH, int nMaxVecs)
#else
float *CHWShader_D3D::mfSetParametersPI(SCGParam *pParams, const int nINParams, float *pDst, EHWShaderClass eSH)
#endif
{
	//regarding snTuner this line causes 30% of the function's time and this function is using 12% of the whole frame time
#if !defined(PS3)
  PROFILE_FRAME(Shader_SetParamsPI);
#endif





  if (!pParams)
    return pDst;

  CD3D9Renderer *const __restrict r = gcpRendD3D;
  SCGParamsPF& RESTRICT_REFERENCE PF = r->m_cEF.m_PF;
  SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;

  register float *pSrc, *pData;
  const SCGParam *__restrict ParamBind = pParams;
  int nParams;
  SEfResTexture *pRT;
  Vec3 v;

  for (int nParam=0; nParam<nINParams; nParam++)
  {




#if defined (DIRECT3D10) || defined(PS3)
    // Not activated yet for this shader
    if (ParamBind->m_dwCBufSlot < 0)
    {
      ParamBind++;
      continue;
    }
#endif

    pSrc = &sData[0].f[0];
    nParams = ParamBind->m_nParameters;
    //uchar* egPrm  = ((uchar*)(ParamBind) + offsetof(SCGParam,m_eCGParamType));
    //int nCurType = *((uchar*)ParamBind); //->m_eCGParamType;
    assert (ParamBind->m_Flags & PF_SINGLE_COMP);

    switch(ParamBind->m_eCGParamType)
    {
      case ECGP_Matr_SI_Obj:
        {
          pData = rRP.m_pCurInstanceInfo->m_Matrix.GetData();





  #if defined(_CPU_SSE) && !defined(_DEBUG)
          sData[0].m128 = _mm_load_ps(&pData[0]);
          sData[1].m128 = _mm_load_ps(&pData[4]);
          sData[2].m128 = _mm_load_ps(&pData[8]);
  #else
          sData[0].f[0] = pData[0]; sData[0].f[1] = pData[1]; sData[0].f[2] = pData[2]; sData[0].f[3] = pData[3];
          sData[1].f[0] = pData[4]; sData[1].f[1] = pData[5]; sData[1].f[2] = pData[6]; sData[1].f[3] = pData[7];
          sData[2].f[0] = pData[8]; sData[2].f[1] = pData[9]; sData[2].f[2] = pData[10]; sData[2].f[3] = pData[11];
  #endif
        }
  #if defined(CRY_DXPS_RASTERTHREAD)
        r->GetD3DDevice()->SoftRast().World(&sData[0].f[0]);
  #endif
        break;
      case ECGP_SI_AmbientOpacity:
        sAmbientOpacity();
        break;
      case ECGP_SI_BendInfo:  



        if (rRP.m_pCurObject->m_pBending)
          sGetBendInfo(r);
        break;
      case ECGP_SI_ObjectAmbColComp:
        sObjectAmbColComp();
        break;

      case ECGP_Matr_PI_Obj_T:
        *(Matrix44A *)(&sData[0].f[0]) = Matrix44A (rRP.m_pCurObject->m_II.m_Matrix);
        break;
      case ECGP_Matr_PI_ViewProj:
        if (!(rRP.m_ObjFlags & FOB_TRANS_MASK))
          ((Matrix44A *)&sData[0])->Transpose(r->m_CameraProjMatrix);
        else
        {
          mathMatrixMultiply_Transp2(&sData[4].f[0], r->m_CameraProjMatrix.GetData(), rRP.m_pCurObject->m_II.m_Matrix.GetData(), g_CpuFlags);
          ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[4]);
        }
        pSrc = &sData[0].f[0];



        break;
      case ECGP_Matr_PI_TCMMatrix:
        pRT = rRP.m_ShaderTexResources[ParamBind->m_nID];
        if (pRT)
          *(Matrix44A *)(&sData[0].f[0]) = pRT->m_TexModificator.m_TexMatrix;
        else
        {
          //assert(0);
          *(Matrix44A *)(&sData[0].f[0]) = r->m_IdentityMatrix;
        }
        break;
      case ECGP_PI_AlphaTest:
        sAlphaTest();
        break;
      case ECGP_PI_Ambient:
        sAmbient(rRP);
        break;
      case ECGP_PI_TextureTileSize:
        sTextureTileSize(rRP);
        break;
      case ECGP_PI_HMAGradients:
        sHMAGradients();
        break;
      case ECGP_PI_AvgFogVolumeContrib:
        sAvgFogVolumeContrib();
        break;
      case ECGP_Matr_PI_Composite:
        {
          ((Matrix44A *)&sData[0])->Multiply(*rRP.m_TI[rRP.m_nProcessThreadID].m_matView->GetTop(), *rRP.m_TI[rRP.m_nProcessThreadID].m_matProj->GetTop());
          ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[0]);
        }
        break;
      case ECGP_PI_MotionBlurData:        
        sGetMotionBlurData(r);
        break;
      case ECGP_PI_CloakParams:
        sGetCloakParams(r);
        break;
      case ECGP_Matr_PI_TexMatrix:
        pSrc = sGetTexMatrix(r, ParamBind);
        break;
      case ECGP_Matr_PI_TCGMatrix:
        pRT = rRP.m_ShaderTexResources[ParamBind->m_nID];
        if (pRT)
          *(Matrix44A *)(&sData[0].f[0]) = pRT->m_TexModificator.m_TexGenMatrix;
        else
        {
          //assert(0);
          *(Matrix44A *)(&sData[0].f[0]) = r->m_IdentityMatrix;
        }
        break;
      case ECGP_PI_ObjColor:
        sData[0].f[0] = rRP.m_pCurInstanceInfo->m_AmbColor[0];
        sData[0].f[1] = rRP.m_pCurInstanceInfo->m_AmbColor[1];
        sData[0].f[2] = rRP.m_pCurInstanceInfo->m_AmbColor[2];
        sData[0].f[3] = rRP.m_pCurInstanceInfo->m_AmbColor[3] * rRP.m_pCurObject->m_fAlpha;
        break;
      case ECGP_PI_Wind:
        sGetWind(r);
        break;
      case ECGP_PI_OSCameraPos:
        {
          ((Matrix44A*)&sData[4])->Transpose(r->m_RP.m_pCurObject->m_II.m_Matrix);
          ((Matrix44A *)&sData[0])->Invert(*(Matrix44A *)&sData[4]);
          TransformPosition(v, r->GetRCamera().Orig, (Matrix44A&)sData);
          sData[0].f[0] = v.x;
          sData[0].f[1] = v.y;
          sData[0].f[2] = v.z;
          sData[0].f[3] = 1.f;
        }
        break;
      case ECGP_PI_VisionParams:    
        sVisionParams();
        break;
			case ECGP_PI_EffectLayerParams:
				sEffectLayerParams();
				break;
      case ECGP_PI_MaterialLayersParams:
        sMaterialLayersParams();
        break;
      case ECGP_PI_FrozenLayerParams:
        sGetFrozenParams(r);
        break;
      case ECGP_PI_NumInstructions:
        sNumInstructions();
        break;
      case ECGP_Matr_PI_OceanMat:
        sOceanMat();
        break;
      default:
        assert(0);
        break;
    }
    if (pSrc)
    {
      if (pDst)
      {









#if defined(_CPU_SSE)
        const __m128 *__restrict cpSrc = (const __m128 *)pSrc;
        __m128 *__restrict cpDst = (__m128 *)pDst;
        const uint32 cParamCnt = nParams;
        for(uint32 i=0; i<cParamCnt; i++)
        {
          cpDst[i] = cpSrc[i];
        }
#else
        const float *const __restrict cpSrc = pSrc;
        float *const __restrict cpDst = pDst;
        const uint32 cParamCnt = nParams;
        for(uint32 i=0; i<cParamCnt; i+=4)
        {
          cpDst[i]	 = cpSrc[i];
          cpDst[i+1] = cpSrc[i+1];
          cpDst[i+2] = cpSrc[i+2];
          cpDst[i+3] = cpSrc[i+3];
        }
#endif
        pDst += cParamCnt*4;
      }
      else
      {
        // in WIN32 pData must be 16 bytes aligned
        assert(!((uint32)pSrc & 0xf) || sizeof(void *)!=4);
#if !defined (DIRECT3D10)



        if (!(ParamBind->m_Flags & PF_INTEGER))
          mfParameterfA(ParamBind, pSrc, nParams, eSH);
        else
          mfParameteri(ParamBind, pSrc, eSH);

#else
        if (!(ParamBind->m_Flags & PF_INTEGER))
          mfParameterfA(ParamBind, pSrc, nParams, eSH, nMaxVecs);
        else
          mfParameteri(ParamBind, pSrc, eSH, nMaxVecs);
#endif
      }
    }
    ParamBind++;
  }
  return pDst;
}

#if defined (DIRECT3D10)
void CHWShader_D3D::mfSetParameters(SCGParam *pParams, const int nINParams, EHWShaderClass eSH, int nMaxVecs)
#else
void CHWShader_D3D::mfSetParameters(SCGParam *pParams, const int nINParams, EHWShaderClass eSH)
#endif
{
  PROFILE_FRAME(Shader_SetParams);





  I3DEngine *pEng;
  CRendElementBase *pRE;
  register float *pSrc, *pData;
  Vec3 v;
  const SCGParam *ParamBind = pParams;
  int nParams;

  if (!pParams)
    return;

  CD3D9Renderer *const __restrict r = gcpRendD3D;
  SCGParamsPF &PF = r->m_cEF.m_PF;
  SRenderPipeline& RESTRICT_REFERENCE rRP = r->m_RP;
	
  for (int nParam=0; nParam<nINParams; nParam++)
  {




#if defined(OPENGL)
    if (ParamBind->m_Flags & PF_MATRIX)
    {
      assert(ParamBind->m_isMatrix != 0);//must be of a matrix type, otherwise cg reports an error
    }
#endif
    pSrc = &sData[0].f[0];
    nParams = ParamBind->m_nParameters;
    //uchar* egPrm  = ((uchar*)(ParamBind) + offsetof(SCGParam,m_eCGParamType));
    //int nCurType = *((uchar*)ParamBind); //->m_eCGParamType;
    for (int nComp=0; nComp<4; nComp++)
    {
#ifdef DO_RENDERLOG
      /*if (CRenderer::CV_r_log >= 3)
      {
        int nCurType = (ParamBind->m_eCGParamType >> (nComp << 3)) & 0xff;
        if (ParamBind->m_Flags & PF_SINGLE_COMP)
          r->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], " Set %s parameter '%s:%s' (%d vectors, reg: %d)\n", sSH[eSH], ParamBind->m_Name.c_str(), r->m_cEF.mfGetShaderParamName((ECGParam)nCurType), nParams, ParamBind->m_dwBind);
        else
          r->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], " Set %s parameter '%s:%s' (%d vectors, reg: %d.%s)\n", sSH[eSH], ParamBind->m_Name.c_str(), r->m_cEF.mfGetShaderParamName((ECGParam)nCurType), nParams, ParamBind->m_dwBind, sComp[nComp]);
      }*/
#endif
      switch((ParamBind->m_eCGParamType >> (nComp << 3)) & 0xff)
      {
      case ECGP_Matr_PF_ViewProjMatrix:
        ((Matrix44A *)&sData[0])->Transpose(r->m_CameraProjMatrix);
#if defined(CRY_DXPS_RASTERTHREAD)
				r->GetD3DDevice()->ViewProjMatrix(pSrc);
#endif
        break;
      case ECGP_Matr_PF_ViewProjZeroMatrix:
        ((Matrix44A *)&sData[0])->Transpose(r->m_CameraProjZeroMatrix);
        break;
      case ECGP_Matr_PB_ViewProjMatrix_I:
        //mathMatrixInverse((float *)&sData[0].f[0], r->m_CameraProjMatrix.GetData(), g_CpuFlags);
        //mathMatrixTranspose(&sData[0].f[0], &sData[0].f[0], g_CpuFlags);        
        ((Matrix44A *)&sData[0])->Invert( *(Matrix44A *)&sData[0] );
        ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[0]);
        break;
      case ECGP_Matr_PB_ViewProjMatrix_IT:
        ((Matrix44A *)&sData[0])->Invert( *(Matrix44A *)&sData[0] );
        //mathMatrixInverse((float *)&sData[0].f[0], r->m_CameraProjMatrix.GetData(), g_CpuFlags);
        break;
      case ECGP_Matr_PB_TerrainBase:
        pSrc = sGetTerrainBase(r);
        break;
      case ECGP_Matr_PB_TerrainLayerGen:
        pSrc = sGetTerrainLayerGen(r);
        break;
      case ECGP_Matr_PB_Temp4_0:
      case ECGP_Matr_PB_Temp4_1:
      case ECGP_Matr_PB_Temp4_2:
      case ECGP_Matr_PB_Temp4_3:
        pSrc = r->m_TempMatrices[ParamBind->m_eCGParamType-ECGP_Matr_PB_Temp4_0][ParamBind->m_nID].GetData();
        break;
      case ECGP_PB_DiffuseMulti:
        sDiffuseMulti();
        break;
      case ECGP_PB_AmbientOpacity:
        sAmbientOpacity();
        break;
      case ECGP_PB_FromRE:
        pRE = rRP.m_pRE;
        if (!pRE || !(pData=(float *)pRE->m_CustomData))
          sData[0].f[nComp] = 0;
        else
          sData[0].f[nComp] = pData[(ParamBind->m_nID>>(nComp*8))&0xff];
        break;
#ifdef USE_PER_MATERIAL_PARAMS
      case ECGP_PM_Tweakable:
      case ECGP_PM_MatDiffuseColor:
      case ECGP_PM_MatSpecularColor:
        sTweakable(ParamBind, eSH);
        break;

      case ECGP_PM_MatEmissiveColor:
        sMatEmissiveColor(ParamBind, eSH);
        break;
#else
      case ECGP_PM_Tweakable:
        assert(ParamBind->m_pData);
        if (ParamBind->m_pData)
        {
          bool bResult = ParamBind->GetTweakable(&sData[0].f[0], nComp);
          assert(bResult == true);
          if (!bResult)
            sData[0].f[nComp] = ParamBind->m_pData->d.fData[nComp];
        }
        break;
      case ECGP_PM_MatDiffuseColor:
        sData[0].f[0] = 1.0f;
        sData[0].f[1] = 1.0f;
        sData[0].f[2] = 1.0f;
        sData[0].f[3] = 1.0f;
        if (pLM=rRP.m_pCurLightMaterial)
        {
          sData[0].f[0] = pLM->m_Diffuse[0];
          sData[0].f[1] = pLM->m_Diffuse[1]; 
          sData[0].f[2] = pLM->m_Diffuse[2];
          sData[0].f[3] = pLM->m_Diffuse[3];
        }

        if (rRP.m_pShaderResources->m_ResFlags & MTL_FLAG_ADDITIVE)
        {
          sData[0].f[0] *= rRP.m_fCurOpacity;
          sData[0].f[1] *= rRP.m_fCurOpacity;
          sData[0].f[2] *= rRP.m_fCurOpacity;
        }

        break;
      case ECGP_PM_MatSpecularColor:
        if (pLM=rRP.m_pCurLightMaterial)
        {
          sData[0].f[0] = pLM->m_Specular[0];
          sData[0].f[1] = pLM->m_Specular[1];
          sData[0].f[2] = pLM->m_Specular[2];
          sData[0].f[3] = max(rRP.m_pCurLightMaterial->m_SpecShininess, 1.0f);
        }
        else
        {
          sOneLine();
        }
        if (rRP.m_pShaderResources->m_ResFlags & MTL_FLAG_ADDITIVE)
        {
          sData[0].f[0] *= rRP.m_fCurOpacity;
          sData[0].f[1] *= rRP.m_fCurOpacity;
          sData[0].f[2] *= rRP.m_fCurOpacity;
        }
        break;
      case ECGP_PM_MatEmissiveColor:
        sData[0].f[0] = 0;
        sData[0].f[1] = 0;
        sData[0].f[2] = 0;
        sData[0].f[3] = 1;
        if (pLM=rRP.m_pCurLightMaterial)
        {
          sData[0].f[0] += pLM->m_Emission[0] * rRP.m_fCurOpacity;
          sData[0].f[1] += pLM->m_Emission[1] * rRP.m_fCurOpacity;
          sData[0].f[2] += pLM->m_Emission[2] * rRP.m_fCurOpacity;

          if (r->CV_r_PostProcess && r->CV_r_nightvision)
          {
            // If nightvision active, brighten up ambient
            if (PF.bPE_NVActive)
            {
              sData[0].f[0] += 0.25f;//0.75f;
              sData[0].f[1] += 0.25f;//0.75f;
              sData[0].f[2] += 0.25f;//0.75f;  
            }
          }

        }
        break;
#endif

      case ECGP_PB_GlobalShaderFlag:
        assert(ParamBind->m_pData);
        if (ParamBind->m_pData)
        {
          if (!rRP.m_pShader)
            pData = NULL;
          else
          {
            bool bVal = (rRP.m_pShader->m_nMaskGenFX & ParamBind->m_pData->d.nData64[nComp]) != 0;
            sData[0].f[nComp] = (float)(bVal);
          }
        }
        break;
      case ECGP_PB_TempData:
        sGetTempData(r, ParamBind);
        break;
      case ECGP_PB_VolumetricFogParams:
        sData[0].f[0] = PF.pVolumetricFogParams.x;
        sData[0].f[1] = PF.pVolumetricFogParams.y;
        sData[0].f[2] = PF.pVolumetricFogParams.z;
        sData[0].f[3] = PF.pVolumetricFogParams.w;
        break;
      case ECGP_PB_VolumetricFogRampParams:
        sData[0].f[0] = PF.pVolumetricFogRampParams.x;
        sData[0].f[1] = PF.pVolumetricFogRampParams.y;
        sData[0].f[2] = PF.pVolumetricFogRampParams.z;
        sData[0].f[3] = PF.pVolumetricFogRampParams.w;
        break;
      case ECGP_PB_VolumetricFogColor:
        sData[0].f[0] = PF.pVolumetricFogColor.x;
        sData[0].f[1] = PF.pVolumetricFogColor.y;
        sData[0].f[2] = PF.pVolumetricFogColor.z;
        sData[0].f[3] = 0.0f;
        break;
      case ECGP_PB_FogColGradColBase:
        sData[0].f[0] = PF.pFogColGradColBase.x;
        sData[0].f[1] = PF.pFogColGradColBase.y;
        sData[0].f[2] = PF.pFogColGradColBase.z;
        sData[0].f[3] = 0.0f;
        break;
      case ECGP_PB_FogColGradColDelta:
        sData[0].f[0] = PF.pFogColGradColDelta.x;
        sData[0].f[1] = PF.pFogColGradColDelta.y;
        sData[0].f[2] = PF.pFogColGradColDelta.z;
        sData[0].f[3] = 0.0f;
        break;
      case ECGP_PB_RuntimeShaderFlag:
        assert(ParamBind->m_pData);
        if (ParamBind->m_pData)
        {
          bool bVal = (rRP.m_FlagsShader_RT & ParamBind->m_pData->d.nData64[nComp]) != 0;
          sData[0].f[nComp] = (float)(bVal);
        }
        break;

      case ECGP_Matr_PB_ProjMatrix:
        //*(Matrix44 *)(&sData[0].f[0]) = r->m_ProjMatrix;
        ((Matrix44A *)&sData[0])->Transpose(r->m_ProjMatrix);
        break;
      case ECGP_Matr_PB_UnProjMatrix:
        ((Matrix44A *)&sData[0])->Multiply(*rRP.m_TI[rRP.m_nProcessThreadID].m_matView->GetTop(), *rRP.m_TI[rRP.m_nProcessThreadID].m_matProj->GetTop());
        ((Matrix44A *)&sData[0])->Invert(*(Matrix44A *)&sData[0]);
        ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[0]);
        break;

      case ECGP_Matr_PB_View_IT:
        ((Matrix44A *)&sData[0])->Multiply(GetTransposed44A(rRP.m_pCurObject->m_II.m_Matrix), r->m_CameraMatrix);
        ((Matrix44A *)&sData[0])->Invert(*(Matrix44A *)&sData[0]);
        break;
      case ECGP_Matr_PB_View:
        ((Matrix44A *)&sData[0])->Multiply(GetTransposed44A(rRP.m_pCurObject->m_II.m_Matrix), r->m_CameraMatrix);
        ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[0]);
        break;
      case ECGP_Matr_PB_View_I:
        ((Matrix44A *)&sData[0])->Multiply(GetTransposed44A(rRP.m_pCurObject->m_II.m_Matrix), r->m_CameraMatrix);
        ((Matrix44A *)&sData[0])->Invert(*(Matrix44A *)&sData[0]);
        ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[0]);
        break;
      case ECGP_Matr_PB_View_T:
        ((Matrix44A *)&sData[0])->Multiply(GetTransposed44A(rRP.m_pCurObject->m_II.m_Matrix), r->m_CameraMatrix);
        break;

      case ECGP_Matr_PB_Camera:
        ((Matrix44A *)&sData[0])->Transpose(r->m_CameraMatrix);
        break;
      case ECGP_Matr_PB_Camera_T:
        pSrc = r->m_CameraMatrix.GetData();
        break;
      case ECGP_Matr_PB_Camera_I:        
        ((Matrix44A *)&sData[0])->Invert(r->m_CameraMatrix);
        ((Matrix44A *)&sData[0])->Transpose(*(Matrix44A *)&sData[0]);
        break;
      case ECGP_Matr_PB_Camera_IT:
        ((Matrix44A *)&sData[0])->Invert(r->m_CameraMatrix);
        break;

      case ECGP_Matr_PB_LightMatrix:
        pSrc = sGetLightMatrix(r);
        break;
      case ECGP_Matr_PB_VoxTerrainAtlasInfo:
        pSrc = sGetVoxTerrainAtlasInfo(r);
        break;
      case ECGP_PL_LightsPos:
        sLightPos();
        break;

      case ECGP_PL_LightsNum:
        assert(0);
        break;

      case ECGP_PL_ShadowMasks:
        sShadowMask();
        break;
      case ECGP_PB_SpecularMulti:
        sSpecularMulti();
        break;
      case ECGP_PL_LDiffuseColors:
        assert(0);
        break;
      case ECGP_PL_LSpecularColors:
        assert(0);
        break;
      case ECGP_PB_LightningPos:
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_POS, v);
        sData[0].f[0] = v.x;
        sData[0].f[1] = v.y;
        sData[0].f[2] = v.z;
        sData[0].f[3] = 0.0f;
        break;
      case ECGP_PB_LightningColSize:
        sLightningColSize();
        break;
      case ECGP_PB_LightsNum:
        sLightsNum(nComp);
        break;
      case ECGP_PB_WaterLevel:
        sData[0].f[0] = PF.vWaterLevel.x;
        sData[0].f[1] = PF.vWaterLevel.y;
        sData[0].f[2] = PF.vWaterLevel.z;
        sData[0].f[3] = 1.0f;
        break;
      case ECGP_PB_HDRDynamicMultiplier:
        sData[0].f[nComp] = PF.fHDRDynamicMultiplier;
        break;
			case ECGP_PB_ObjVal:
				{
					SRenderObjData *pOD = rRP.m_pCurObject->GetObjData(rRP.m_nProcessThreadID);
					if (pOD)
					{
						pData = (float *)pOD->m_fTempVars;
						sData[0].f[nComp] = pData[(ParamBind->m_nID>>(nComp*8))&0xff];
					}
				}
				break;
			case ECGP_PB_OutdoorAOParams:
				{
					SRenderObjData *pOD = rRP.m_pCurObject->GetObjData(rRP.m_nProcessThreadID);
					if (pOD)
					{
						float * pObjTmpVars = pOD->m_fTempVars;
						sData[0].f[0] = gEnv->p3DEngine->GetSkyBrightness();
						sData[0].f[1] = pObjTmpVars[2]; // outdoor AO texgen scale
						sData[0].f[2] = pObjTmpVars[4]-pObjTmpVars[3]; // z range
						sData[0].f[3] = 0.f;//unused
					}
				}
				break;
      case ECGP_PB_RotGridScreenOff:
        sGetRotGridScreenOff(r);
        break;
      case ECGP_PB_GlowParams:
        // to be merged with glow color/diffuse/emissive in future
        if (rRP.m_pShaderResources)
        {
          sData[0].f[0] = rRP.m_pShaderResources->Glow(); 

					if( /*CRenderer::CV_r_glow == 2 && */gRenDev->IsHDRModeEnabled() )
					{						
						float fPowFactor = gEnv->p3DEngine->GetHDRDynamicMultiplier();
						if (gRenDev->IsLinearSpaceShadingEnabled())
							sData[0].f[0] *= fPowFactor;

						sData[0].f[0] = powf( sData[0].f[0], fPowFactor);
					}

          sData[0].f[1] = sData[0].f[0];
          sData[0].f[2] = sData[0].f[0];
          sData[0].f[3] = 1.0f;
        }
        else
          sZeroLine();
        break;
      case ECGP_PB_FurParams:
        sFurParams();
        break;
      case ECGP_PB_IrregKernel:
        sGetIrregKernel(r);
        break;
      case ECGP_PB_RegularKernel:
        sGetRegularKernel(r);
        break;
      case ECGP_PB_DeformWaveX:
        {
          if (rRP.m_pShaderResources && rRP.m_pShaderResources->m_pDeformInfo)
          {
            SDeformInfo *di = rRP.m_pShaderResources->m_pDeformInfo;
            if (di->m_fDividerX != 0)
            {
              sData[0].f[0] = rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime*di->m_WaveX.m_Freq+di->m_WaveX.m_Phase;
              sData[0].f[1] = di->m_WaveX.m_Amp;
              sData[0].f[2] = di->m_WaveX.m_Level;
              sData[0].f[3] = 1.0f / di->m_fDividerX;
            }
            else
              sIdentityLine();
          }
          else
            sIdentityLine();
        }
        break;
      case ECGP_PB_DeformWaveY:
        {
          if (rRP.m_pShaderResources && rRP.m_pShaderResources->m_pDeformInfo)
          {
            SDeformInfo *di = r->m_RP.m_pShaderResources->m_pDeformInfo;
            if (di->m_fDividerY != 0)
            {
              sData[0].f[0] = rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime*di->m_WaveY.m_Freq+di->m_WaveY.m_Phase;
              sData[0].f[1] = di->m_WaveY.m_Amp;
              sData[0].f[2] = di->m_WaveY.m_Level;
              sData[0].f[3] = 1.0f / di->m_fDividerY;
            }
            else
              sIdentityLine();
          }
          else
            sIdentityLine();
        }
        break;
			case ECGP_PB_DeformFrequencies:
				{
					SRenderShaderResources *const __restrict pSR = rRP.m_pShaderResources;
					if (pSR && pSR->m_pDeformInfo)
					{
						SDeformInfo *const __restrict di = pSR->m_pDeformInfo;    
						sZeroLine();

						float &fRealtime = rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime;
						if (di->m_fDividerX != 0)
							sData[0].f[0] = fRealtime * di->m_WaveX.m_Freq + di->m_WaveX.m_Phase;
						if (di->m_fDividerY != 0)
							sData[0].f[1] = fRealtime * di->m_WaveY.m_Freq + di->m_WaveY.m_Phase;
						if (di->m_fDividerZ != 0)
							sData[0].f[2] = fRealtime * di->m_WaveZ.m_Freq + di->m_WaveZ.m_Phase;
						if (di->m_fDividerW != 0)    
							sData[0].f[3] = fRealtime * di->m_WaveW.m_Freq + di->m_WaveW.m_Phase;
					}
					else
						sZeroLine();
				}
				break;

			case ECGP_PB_DeformAmplitudes:
				{
					SRenderShaderResources *const __restrict pSR = rRP.m_pShaderResources;
					if (pSR && pSR->m_pDeformInfo)
					{
						SDeformInfo *const __restrict di = pSR->m_pDeformInfo;
						sZeroLine();

						if (di->m_fDividerX != 0)
							sData[0].f[0] = di->m_WaveX.m_Amp;
						if (di->m_fDividerY != 0)
							sData[0].f[1] = di->m_WaveY.m_Amp;
						if (di->m_fDividerZ != 0)
							sData[0].f[2] = di->m_WaveZ.m_Amp;
						if (di->m_fDividerW != 0)
							sData[0].f[3] = di->m_WaveW.m_Amp;
					}
					else
						sZeroLine();
				}
				break;

			case ECGP_PB_DeformLevels:
				{
					SRenderShaderResources *const __restrict pSR = rRP.m_pShaderResources;
					if (pSR && pSR->m_pDeformInfo)
					{
						SDeformInfo *const __restrict di = pSR->m_pDeformInfo;
						sZeroLine();

						if (di->m_fDividerX != 0)
							sData[0].f[0] = di->m_WaveX.m_Level;
						if (di->m_fDividerY != 0)
							sData[0].f[1] = di->m_WaveY.m_Level;
						if (di->m_fDividerZ != 0)
							sData[0].f[2] = di->m_WaveZ.m_Level;
						if (di->m_fDividerW != 0)
							sData[0].f[3] = di->m_WaveW.m_Level;
					}
					else
						sZeroLine();
				}
				break;

			case ECGP_PB_DeformVertexPhases:
				{
					SRenderShaderResources *const __restrict pSR = rRP.m_pShaderResources;
					if (pSR && pSR->m_pDeformInfo)
					{
						SDeformInfo *const __restrict di = pSR->m_pDeformInfo;
						sZeroLine();

						if (di->m_fDividerX != 0)
							sData[0].f[0] = 1.0f / di->m_fDividerX;
						if (di->m_fDividerY != 0)
							sData[0].f[1] = 1.0f / di->m_fDividerY;
						if (di->m_fDividerZ != 0)
							sData[0].f[2] = 1.0f / di->m_fDividerZ;
						if (di->m_fDividerW != 0)
							sData[0].f[3] = 1.0f / di->m_fDividerW;
					}
					else
						sZeroLine();
				}
				break;
      case ECGP_PB_DeformBend:
        if (rRP.m_pShaderResources && rRP.m_pShaderResources->m_pDeformInfo)
        {
          SDeformInfo *di = rRP.m_pShaderResources->m_pDeformInfo;
          sData[0].f[0] = CShaderMan::EvalWaveForm(&di->m_WaveX);
          sData[0].f[1] = CShaderMan::EvalWaveForm(&di->m_WaveY);
          sData[0].f[2] = di->m_fDividerX;
          sData[0].f[3] = di->m_fDividerY;
        }
        else
          sIdentityLine();
        break;
      case ECGP_PB_DeformNoiseInfo:
        if (rRP.m_pShaderResources && rRP.m_pShaderResources->m_pDeformInfo)
        {
          SDeformInfo *di = r->m_RP.m_pShaderResources->m_pDeformInfo;
          sData[0].f[0] = di->m_vNoiseScale[0] * rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime;
          sData[0].f[1] = di->m_vNoiseScale[1] * rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime;
          sData[0].f[2] = di->m_vNoiseScale[2] * rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime;
          sData[0].f[3] = 1.0f;
        }
        else
          sIdentityLine();
        break;
      case ECGP_PB_TFactor:
        sData[0].f[0] = r->m_RP.m_CurGlobalColor[0];
        sData[0].f[1] = r->m_RP.m_CurGlobalColor[1];
        sData[0].f[2] = r->m_RP.m_CurGlobalColor[2];
        sData[0].f[3] = r->m_RP.m_CurGlobalColor[3];
        break;
      case ECGP_PB_RandomParams:
        {
          sData[0].f[0] = Random();
          sData[0].f[1] = Random();
          sData[0].f[2] = Random();
          sData[0].f[3] = Random();
        }
        break;
      case ECGP_Matr_SG_ShadowProj_0:
        pSrc = r->m_TempMatrices[0][0].GetData();
        break;
      case ECGP_Matr_SG_ShadowProj_1:
        pSrc = r->m_TempMatrices[1][0].GetData();
        break;
      case ECGP_Matr_SG_ShadowProj_2:
        pSrc = r->m_TempMatrices[2][0].GetData();
        break;
      case ECGP_Matr_SG_ShadowProj_3:
        pSrc = r->m_TempMatrices[3][0].GetData();
        break;
      case ECGP_SG_FrustrumInfo:
        sData[0].f[0] = rRP.m_TI[rRP.m_nProcessThreadID].m_vFrustumInfo.x;
        sData[0].f[1] = rRP.m_TI[rRP.m_nProcessThreadID].m_vFrustumInfo.y;
        sData[0].f[2] = rRP.m_TI[rRP.m_nProcessThreadID].m_vFrustumInfo.z;
        sData[0].f[3] = rRP.m_TI[rRP.m_nProcessThreadID].m_vFrustumInfo.w;
        break;
      case ECGP_PB_DecalZFightingRemedy:
        sData[0].f[0] = PF.pDecalZFightingRemedy.x;
        sData[0].f[1] = PF.pDecalZFightingRemedy.y;
        sData[0].f[2] = PF.pDecalZFightingRemedy.z;
        sData[0].f[3] = 0;
        break;
      case ECGP_PB_CameraFront:
        sCameraFront(r);
        break;
      case ECGP_PB_CameraRight:
        sCameraRight(r);
        break;
      case ECGP_PB_CameraUp:
        sCameraUp(r);
        break;
      case ECGP_PB_RTRect:
        sRTRect(r);
        break;
      case ECGP_PB_SkyLightHazeColorPartialMieInScatter:
        sSkyLightHazeColorPartialMieInScatter(PF);
        break;
      case ECGP_PB_SkyLightHazeColorPartialRayleighInScatter:
        sSkyLightHazeColorPartialRayleighInScatter(PF);
        break;
      case ECGP_PB_SkyLightSunDirection:
        sSkyLightSunDirection(r);
        break;
      case ECGP_PB_SkyLightPhaseFunctionConstants:
        sSkyLightPhaseFunctionConstants(r);
        break;
#if !defined(XENON) && !defined(PS3)
      case ECGP_PB_LightInfoTC:
        {
          SLightPass *pLP = &rRP.m_LPasses[rRP.m_nCurLightPass];
          if (pLP->nLights)
          {
            int nGroup = pLP->pLights[0]->m_Id >> 2;
            sData[0].f[1] = (float)nGroup / 8;

            // Fast lookup to pre-build table
            int nID = 0;
            for (uint32 i=0; i<pLP->nLights; i++)
            {
              CDLight *pDL = pLP->pLights[i];
              assert ((pDL->m_Id >> 2) == nGroup);
              nID |= (pDL->m_Id&3) << (i*2);
            }
            Vec4 &Data = r->m_RP.m_LightInfo[nID];
            sData[0].f[0] = Data[0];
            sData[0].f[2] = Data[2];
            sData[0].f[3] = Data[3];
            assert(sData[0].f[0] >= 0.0f);
          }
        }
        break;
#endif
      case ECGP_PB_ResourcesOpacity:
        if (rRP.m_pShaderResources)
        {
          sData[0].f[0] = r->m_RP.m_pShaderResources->Opacity();
          sData[0].f[1] = sData[0].f[0];
          sData[0].f[2] = sData[0].f[0];
          sData[0].f[3] = sData[0].f[0];
        }
        else
          sZeroLine();
        break;
      case ECGP_PB_Scalar:
        assert(ParamBind->m_pData);
        if (ParamBind->m_pData)
          sData[0].f[nComp] = ParamBind->m_pData->d.fData[nComp];
        break;

      case ECGP_PB_CausticsParams:
        sData[0].f[0] = CRenderer::CV_r_watercausticsdistance;//PF.pCausticsParams.x;
        sData[0].f[1] = PF.pCausticsParams.y;
        sData[0].f[2] = PF.pCausticsParams.z;
        sData[0].f[3] = 1.0f;
        break;

      case ECGP_PF_SunColor:
        {
          pEng = gEnv->p3DEngine;
          v = pEng->GetSunColor();
          sData[0].f[0] = v.x;
          sData[0].f[1] = v.y;
          sData[0].f[2] = v.z;
          sData[0].f[3] = 1.0f / (r->m_fAdaptedSceneScaleLBuffer + FLT_MIN);
          break;
        }
      case ECGP_PF_SkyColor:
        sSkyColor();
        break;

      case ECGP_PB_CausticsSmoothSunDirection:
        sCausticsSmoothSunDirection();
        break;

      case ECGP_PF_SunDirection:
        sSunDirection();
        break;
      case ECGP_PF_FogColor:
        sGetVolumetricFogParams(r);
        sData[0].f[3] = sData[0].f[2];
        sData[0].f[0] = rRP.m_TI[rRP.m_nProcessThreadID].m_FS.m_CurColor[0];
        sData[0].f[1] = rRP.m_TI[rRP.m_nProcessThreadID].m_FS.m_CurColor[1];
        sData[0].f[2] = rRP.m_TI[rRP.m_nProcessThreadID].m_FS.m_CurColor[2];
        //sData[0].f[3] = r->m_fAdaptedSceneScale;
        break;
      case ECGP_PF_CameraPos:
        v = r->GetRCamera().Orig;
        sData[0].f[0] = v.x;
        sData[0].f[1] = v.y;
        sData[0].f[2] = v.z;
        sData[0].f[3] = 1.f;
        break;
      case ECGP_PF_ScreenSize:
        sGetScreenSize(r);
        break;
      case ECGP_PF_Time:
        //sData[0].f[nComp] = r->m_RP.m_ShaderCurrTime; //r->m_RP.m_RealTime;
        sData[0].f[nComp] = rRP.m_TI[rRP.m_nProcessThreadID].m_RealTime;
        assert(ParamBind->m_pData);
        if (ParamBind->m_pData)
          sData[0].f[nComp] *= ParamBind->m_pData->d.fData[nComp];
        break;
      case ECGP_PF_ProjRatio:
        {
          const CRenderCamera& rc = r->GetRCamera();
          float zn = rc.Near;
          float zf = rc.Far; 
          sData[0].f[0] = zf/(zf-zn);
          sData[0].f[1] = zn/(zn-zf);
          sData[0].f[2] = 0.0f; 
          sData[0].f[3] = 1.0f;
        }
        break;
      case ECGP_PF_DepthFactor:
        sDepthFactor(r);
        break;
      case ECGP_PF_NearFarDist:
        sNearFarDist(r);
        break;
#ifndef EXCLUDE_SCALEFORM_SDK
      case ECGP_Matr_PB_SFCompMat:
        sSFCompMat();
        break;
      case ECGP_Matr_PB_SFTexGenMat0:
        sSFTexGenMat0(r);
        break;
      case ECGP_Matr_PB_SFTexGenMat1:
        sSFTexGenMat1(r);
        break;
      case ECGP_PB_SFBitmapColorTransform:
        sSFBitmapColorTransform(r);
        break;
      case ECGP_PB_SFSRGBFixup:
        sData[0].f[0] = gcpRendD3D->CV_r_useSRGB ? 1.8f : 1.0f;
        sData[0].f[1] = sData[0].f[2] = sData[0].f[3] = 0;
        break;
#endif
      case ECGP_PB_CloudShadingColorSun:
        sData[0].f[0] = PF.pCloudShadingColorSun.x;
        sData[0].f[1] = PF.pCloudShadingColorSun.y;
        sData[0].f[2] = PF.pCloudShadingColorSun.z;
        sData[0].f[3] = 0;
        break;

      case ECGP_PB_CloudShadingColorSky:
        sData[0].f[0] = PF.pCloudShadingColorSky.x;
        sData[0].f[1] = PF.pCloudShadingColorSky.y;
        sData[0].f[2] = PF.pCloudShadingColorSky.z;
        sData[0].f[3] = 0;
        break;

      case ECGP_PB_AlphaTest:
        sData[0].f[0] = 0;
        sData[0].f[1] = 0;
        sData[0].f[2] = 0;
        sData[0].f[3] = rRP.m_pShaderResources ? rRP.m_pShaderResources->m_AlphaRef : 0;
        // specific condition for hair zpass
        if ((rRP.m_pShader->m_Flags2 & EF2_HAIR) && !(rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
          sData[0].f[3] = 0.51f;
        break;
			case ECGP_PB_ResInfoDiffuse:
        sResInfoDiffuse();
  			break;
			case ECGP_PB_ResInfoBump:
        sResInfoBump();
  			break;
			case ECGP_PB_TexelDensityParam:
        sTexelDensityParam();
  			break;
			case ECGP_PB_TexelDensityColor:
        sTexelDensityColor();
  			break;
			case ECGP_PB_TexelsPerMeterInfo:
        sTexelsPerMeterInfo();
				break;
			case ECGP_PB_ColorCharts_WidthFixPS3:
			{




				assert(0);
				sData[0].f[0] = 1;

				sData[0].f[1] = sData[0].f[2] = sData[0].f[3] = 0;
				break;
			}
      case 0:
        break;

      default:
        assert(0);
        break;
        //Warning("Unknown Parameter '%s' of type %d", ParamBind->m_Name.c_str(), ParamBind->m_eCGParamType);
        //assert(0);
        //return NULL;
      }
      if (ParamBind->m_Flags & PF_SINGLE_COMP)
        break;
    }
    if (pSrc)
    {
      // in WIN32 pData must be 16 bytes aligned
      assert(!((uint32)pSrc & 0xf) || sizeof(void *)!=4);
#if !defined (DIRECT3D10)



      if (!(ParamBind->m_Flags & PF_INTEGER))
        mfParameterfA(ParamBind, pSrc, nParams, eSH);
      else
        mfParameteri(ParamBind, pSrc, eSH);

#else
      if (!(ParamBind->m_Flags & PF_INTEGER))
        mfParameterfA(ParamBind, pSrc, nParams, eSH, nMaxVecs);
      else
        mfParameteri(ParamBind, pSrc, eSH, nMaxVecs);
#endif
    }
    ++ParamBind;
  }
}


//=========================================================================================

void CHWShader_D3D::mfReset(uint32 CRC32)
{
  for (int i=0; i<m_Insts.size(); i++)
  {
    m_pCurInst = &m_Insts[i];
    if (!m_pCurInst->m_bDeleted)
      m_pCurInst->Release(m_pCache);
  }
  m_pCurInst = NULL;
  m_Insts.clear();
  if (CRC32!=0 && gRenDev->m_cEF.m_nCombinationsProcess < 0)
  {
    // Delete all shared instances for this shader if time is different
    InstanceMapItor itInst = m_SharedInsts.find(m_EntryFunc);
    SHWSSharedList *pInstSH = NULL;
    if (itInst != m_SharedInsts.end())
    {
      pInstSH = itInst->second;
      int i, j;
      const char *nm = gRenDev->m_RP.m_pShader->m_NameShader.c_str();
      SHWSSharedName *pSHN;
      for (i=0; i<pInstSH->m_SharedNames.size(); i++)
      {
        pSHN = &pInstSH->m_SharedNames[i];
        if (!stricmp(pSHN->m_Name.c_str(), nm))
          break;
      }
      if (i != pInstSH->m_SharedNames.size())
      {
        if (pSHN->m_CRC32 != CRC32)
        {
          pSHN->m_CRC32 = CRC32;
          for (i=0; i<pInstSH->m_SharedInsts.size(); i++)
          {
            SHWSSharedInstance *pSHI = &pInstSH->m_SharedInsts[i];
            for (j=0; j<pSHI->m_Insts.size(); j++)
            {
              m_pCurInst = &pSHI->m_Insts[j];
              m_pCurInst->Release(m_pCache);
            }
            pSHI->m_Insts.clear();
          }
          pInstSH->m_SharedInsts.clear();
        }
      }
      else
      {
        int nnn = 0;
      }
    }
  }

  mfCloseCacheFile();
}

CHWShader_D3D::~CHWShader_D3D()
{
  mfFree(0);
}

bool CHWShader_D3D::mfSetSamplers()
{
  //PROFILE_FRAME(Shader_SetShaderSamplers);
  SHWSInstance *__restrict pInst = m_pCurInst;
  if (!pInst)
    return false;
  const uint32 nSize=pInst->m_pSamplers.size();
  if (!nSize)
    return true;
  CD3D9Renderer *__restrict rd = gcpRendD3D;
  SRenderShaderResources *__restrict pSR = rd->m_RP.m_pShaderResources;

  uint32 i;
  STexSampler *pSamp = &pInst->m_pSamplers[0];
  for (i=0; i<nSize; i++, pSamp++)
  {
    CTexture *tx = pSamp->m_pTex;
    assert(tx);
    if (!tx)
      continue;
    //int nSetID = -1;
    int nTexSlot = -1;
    int nSamplerSlot = pSamp->m_nSamplerSlot;
    assert(nSamplerSlot >= 0);
    STexSampler *pSM = pSamp;
    int nTS = pSM->m_nTexState;
#if defined (DIRECT3D9) || defined(PS3)
    CTexture::s_CurStage = nSamplerSlot;
#else
    CTexture::s_CurStage = i;
#endif
    if (tx >= &CTexture::s_ShaderTemplates[0] && tx <= &CTexture::s_ShaderTemplates[EFTT_MAX-1])
    {
      /*if (tx >= &CTexture::s_ShaderTemplates[EFTT_LIGHTMAP] && tx <= &CTexture::s_ShaderTemplates[EFTT_OCCLUSION])
      {
        if (tx == &CTexture::s_ShaderTemplates[EFTT_RAE] )
          nSetID = rd->m_RP.m_pCurObject->m_nRAEId ? rd->m_RP.m_pCurObject->m_nRAEId : CTexture::s_ptexWhite->GetID();
      }
      else*/
      {
        nTexSlot = (int)(tx - &CTexture::s_ShaderTemplates[0]);

        if (!pSR || !pSR->m_Textures[nTexSlot])
        {
          tx = CTexture::s_ptexNoTexture;           

          if (nTexSlot)
          {
            // if no texture, pass dummy textures (to minimize shader permutations)
            tx = CTexture::s_ptexWhite;

            if(nTexSlot == EFTT_DECAL_OVERLAY )
            {
              tx = CTexture::s_ptexGray;
            }
            else
            if (nTexSlot == EFTT_BUMP)
            {
              tx = CTexture::s_ptexFlatBump;
            }
            else
            if (nTexSlot == EFTT_BUMP_DIFFUSE)
            {
              if (pSR && pSR->m_Textures[EFTT_BUMP]) 
              {
                pSM = &pSR->m_Textures[EFTT_BUMP]->m_Sampler;
                tx = pSM->m_pTex;
              }
              else
                tx = CTexture::s_ptexFlatBump;
            }
          }
        }
        else
        {
					pSM = &pSR->m_Textures[nTexSlot]->m_Sampler;
					tx = pSM->m_pTex;

          if (nTS<0 || !CTexture::s_TexStates[nTS].m_bActive)
            nTS = pSM->m_nTexState;   // Use material texture state
          if (pSM->m_pDynTexSource)
          {
            if (pSM->m_pDynTexSource->Apply(-1, nTS))
              continue;
            else
              tx = CTexture::s_ptexWhite;
          }
        }
      }
    }
    if (pSM && pSM->m_pAnimInfo)
      pSM->Update();
    //    assert(tx);
    if (!tx)
      continue;

    /*if (nSetID > 0)
    {
      CTexture::ApplyForID(nSetID, nTS, nSamplerSlot);
    }
    else*/
    {
      int nCustomID = tx->GetCustomID();
      if (nCustomID <= 0)
      {
				if (tx->UseDecalBorderCol())
				{
					STexState TS = CTexture::s_TexStates[nTS];
					//TS.SetFilterMode(...); // already set up
#if !defined(XENON)
					TS.SetClampMode(TADDR_BORDER, TADDR_BORDER, TADDR_BORDER);
					TS.SetBorderColor(ColorF(1,1,1,0).pack_argb8888());


#endif
					nTS = CTexture::GetTexState(TS);
				}

        tx->Apply(-1, nTS, nTexSlot, nSamplerSlot);
      }
      else
      switch (nCustomID)
      {
        case TO_FROMRE0:
        case TO_FROMRE1:
          {
            if (rd->m_RP.m_pRE)
              nCustomID = rd->m_RP.m_pRE->m_CustomTexBind[nCustomID-TO_FROMRE0];
            else
              nCustomID = rd->m_RP.m_RECustomTexBind[nCustomID-TO_FROMRE0];
            if (nCustomID < 0)
              break;

            CTexture *pTex = CTexture::GetByID(nCustomID);
            pTex->Apply(-1, nTS, nTexSlot, nSamplerSlot);

            //CTexture::ApplyForID(nCustomID, bSRGB);
          }
          break;

        case TO_ZTARGET_MS:
          {
            CTexture *pTex = CTexture::s_ptexZTarget;
            assert(pTex);
            if (pTex)
              pTex->Apply(-1, nTS, nTexSlot, nSamplerSlot, 4);
          }
          break;
				case TO_SCENE_NORMALMAP:
					{
						CTexture *pTex = CTexture::s_ptexSceneNormalsMap;
						assert(pTex);
						if (pTex)
							pTex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
					}
					break;

        case TO_SHADOWID0:
        case TO_SHADOWID1:
        case TO_SHADOWID2:
        case TO_SHADOWID3:
        case TO_SHADOWID4:
        case TO_SHADOWID5:
        case TO_SHADOWID6:
        case TO_SHADOWID7:
          {
#if defined (DIRECT3D10)
            //TF reset custom res view after shadow pass
            int nCustomResViewID = rd->m_RP.m_ShadowCustomResViewID[nCustomID-TO_SHADOWID0];
#endif
            nCustomID = rd->m_RP.m_ShadowCustomTexBind[nCustomID-TO_SHADOWID0];

            if (nCustomID < 0)
              break;
            //force  MinFilter = Linear; MagFilter = Linear; for HW_PCF_FILTERING
            STexState TS = CTexture::s_TexStates[nTS];
            TS.m_pDeviceState = NULL;

            if (gRenDev->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ])
            {
              TS.SetFilterMode(FILTER_LINEAR);
#if defined (DIRECT3D10)
              if (nCustomResViewID>=0)
              {
                //texture array case
                TS.SetComparisonFilter(false);
              }
              else
              {
                //non texture array case
                TS.SetComparisonFilter(true);
              }

#endif
            }
            else
            {
              if (gRenDev->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[ HWSR_SHADOW_FILTER ])
              {
                TS.SetFilterMode(FILTER_LINEAR);
              }
              else
              {
                TS.SetFilterMode(FILTER_POINT);
              }
            }


            //TS.PostCreate();

            CTexture* tex  = CTexture::GetByID(nCustomID);
#if defined (DIRECT3D10)
            tex->Apply(-1, CTexture::GetTexState(TS), nTexSlot, nSamplerSlot, nCustomResViewID);
#else
            tex->Apply(-1, CTexture::GetTexState(TS), nTexSlot, nSamplerSlot);
#endif

          }
          break;

        case TO_FROMRE0_FROM_CONTAINER:
        case TO_FROMRE1_FROM_CONTAINER:
          {
            // take render element from vertex container render mesh if available
            CRendElementBase *pRE = sGetContainerRE0(rd->m_RP.m_pRE);
            if (pRE)
              nCustomID = pRE->m_CustomTexBind[nCustomID-TO_FROMRE0_FROM_CONTAINER];
            else
              nCustomID = rd->m_RP.m_RECustomTexBind[nCustomID-TO_FROMRE0_FROM_CONTAINER];
            if (nCustomID < 0)
              break;

            CTexture::ApplyForID(nCustomID, nTS, nSamplerSlot);
          }
          break;

        case TO_LIGHTINFO:
          {
            CTexture *pTex = CTexture::s_ptexLightInfo[SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID]-1];
            assert(pTex);
            if (pTex)
              pTex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          }
          break;

        case TO_SCREENSHADOWMAP:
          {
            CTexture *tex = NULL;
            int nCurLightGroup = rd->m_RP.m_nCurLightGroup;
            if( rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_RAINPASS ) // special case for rain pass - make sure light group is 0
              nCurLightGroup = 0;

            if (nCurLightGroup>= 0)
            {
              int nGroup = nCurLightGroup>=0 ? nCurLightGroup : MAX_REND_LIGHT_GROUPS;
              if (SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID]-1][nGroup])
              {
                assert(nCurLightGroup>=0 && nCurLightGroup<MAX_REND_LIGHTS/4);
                //tex = CTexture::m_Text_ScreenShadowMap[rd->m_RP.m_nCurLightGroup];
                tex  = CTexture::s_ptexCurrentScreenShadowMap[nCurLightGroup];
              }
              else
              {
                tex = CTexture::s_ptexBlack;
              }
            }
            else
            {
              tex = CTexture::s_ptexBlack;
            }
            assert(tex);
            if (tex)
              tex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          }
          break;

				case TO_SCATTER_LAYER:
					{
						CTexture *tex = NULL;
						if ( CTexture::s_ptexScatterLayer )
							tex = CTexture::s_ptexScatterLayer;
						else
						{
							tex = CTexture::s_ptexBlack;
						}

						assert(tex);
						if (tex)
							tex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
					}
					break;

        case TO_SCENE_DIFFUSE_ACC:
          {
            CTexture *tex = NULL;
            if ( CDeferredShading::Instance().GetLightsCount() ) //|| CDeferredShading::Instance().DiffuseLBufferUpdated())
              tex = CTexture::s_ptexCurrentSceneDiffuseAccMap;
            else
              tex = CTexture::s_ptexBlack;

            assert(tex);
            if (tex)
              tex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          }
          break;

        case TO_SCENE_SPECULAR_ACC:
          {
            CTexture *tex = CTexture::s_ptexBlack;
#if !XENON_FORCE_720P
            if ( CDeferredShading::Instance().GetLightsCount() )
              tex = CTexture::s_ptexSceneSpecularAccMap; //(!CRenderer::CV_r_deferredshadinginterleavedacc)? CTexture::s_ptexSceneTarget : CTexture::s_ptexSceneSpecularAccMap;
#endif
            assert(tex);
            if (tex)
              tex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          }
          break;

        case TO_SCENE_TARGET:
          {
            CTexture *tex = CTexture::s_ptexCurrSceneTarget;
            if( !tex )
              tex = CTexture::s_ptexWhite;

            tex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          }
          break;

				case TO_SCREEN_AO_TARGET:
					{
						CTexture *tex = NULL;
						if ((rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ALLOW_AO) && rd->m_RP.m_nAOMaskUpdateLastFrameId == rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID && CTexture::s_ptexAOTarget->GetDevTexture() )
							tex = CTexture::s_ptexAOTarget;
						else
							tex = CTexture::s_ptexWhite;
						assert(tex);
						if (tex)
							tex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
					}
					break;

        case TO_DOWNSCALED_ZTARGET_FOR_AO:
          {
            CTexture *tex = NULL;
            if (rd->CV_r_SSAO_downscale_ztarget)
              tex = CTexture::s_ptexZTargetScaled;
            else
              tex = CTexture::s_ptexZTarget;
            assert(tex);
            if (tex)
              tex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          }
          break;

        case TO_FROMOBJ:
          {
            if (rd->m_RP.m_pCurObject)
              nCustomID = rd->m_RP.m_pCurObject->m_nTextureID;
            if (nCustomID <= 0)
              return 0;

            //CTexture::ApplyForID(nCustomID, bSRGB, nTS);
            CTexture *pTex = CTexture::GetByID(nCustomID);
            pTex->Apply(-1, nTS, nTexSlot, nSamplerSlot);

          }
          break;

        case TO_FROMLIGHT:
          {
            bool bRes = CTexture::SetProjector(-1, nTS, nSamplerSlot);
            if (!bRes && !(rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MULTILIGHTS))
              Warning( "Couldn't set projected texture for light source (Shader: '%s')\n", rd->m_RP.m_pShader->GetName());
          }
          break;

        case TO_RT_CM:
          {
            SHRenderTarget *pRT = pSM->m_pTarget ? pSM->m_pTarget : pSamp->m_pTarget;
            assert(pRT);
            if (!pRT)
              break;
            SEnvTexture *pEnvTex = pRT->GetEnvCM();
            assert(pEnvTex->m_pTex);
            if (pEnvTex && pEnvTex->m_pTex)
              pEnvTex->m_pTex->Apply(-1, nTS);
          }
          break;

        case TO_RT_2D:
          {
            SHRenderTarget *pRT = pSM->m_pTarget ? pSM->m_pTarget : pSamp->m_pTarget;
            SEnvTexture *pEnvTex = pRT->GetEnv2D();
            //assert(pEnvTex->m_pTex);
            if (pEnvTex && pEnvTex->m_pTex)
              pEnvTex->m_pTex->Apply(-1, nTS);
          }
          break;

        case TO_SCREENMAP:
          //if (rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR)
          //  CTexture::m_Text_ScreenMap_HDR->Apply(-1, nTS);
          //else
            CTexture::s_ptexScreenMap->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          break;

        case TO_WATEROCEANMAP:
            CTexture::s_ptexWaterOcean->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          break;

        case TO_WATERVOLUMEMAP:
          {
            if( CTexture::s_ptexWaterVolumeDDN )
            {
              using namespace NPostEffects;
              static CEffectParam *pParam = PostEffectMgr().GetByName("WaterVolume_Amount"); 
              assert(pParam && "Parameter doesn't exist");

              // Activate puddle generation
              if( pParam )
                pParam->SetParam(1.0f);   

              CTexture::s_ptexWaterVolumeDDN->Apply(-1, nTS, nTexSlot, nSamplerSlot);
            }
            else
              CTexture::s_ptexFlatBump->Apply( -1 );


            //gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
            //CTexture::m_Text_WaterVolume->Apply(-1, nTS, nTexSlot, nSamplerSlot);
          }
          
          break;

        case TO_WATERPUDDLESMAP:
          {
            if( CTexture::s_ptexWaterPuddlesDDN )
            {
              using namespace NPostEffects;
              static CEffectParam *pParam = PostEffectMgr().GetByName("WaterPuddles_Amount"); 
              assert(pParam && "Parameter doesn't exist");

              // Activate puddle generation
              if( pParam )
                pParam->SetParam(1.0f);   

              CTexture::s_ptexWaterPuddlesDDN->Apply(-1, nTS, nTexSlot, nSamplerSlot);
            }
            else
              CTexture::s_ptexWhite->Apply( -1 );

          }
          break;

        case TO_TERRAIN_LM:
          {
            // do funky stuff
            static ICVar* e_shadows( gEnv->pConsole->GetCVar( "e_Shadows" ) );
            bool setupTerrainShadows( e_shadows->GetIVal() > 0 );

            if( setupTerrainShadows )
            {
              // terrain shadow map
              ITerrain* pTerrain( gEnv->p3DEngine->GetITerrain() );
              Vec4 texGenInfo;
              int terrainLightMapID( pTerrain->GetTerrainLightmapTexId( texGenInfo ) );
              CTexture* pTerrainLightMap( terrainLightMapID > 0 ? CTexture::GetByID( terrainLightMapID ) : CTexture::s_ptexWhite );
              assert( pTerrainLightMap );

              STexState pTexStateLinearClamp;
              pTexStateLinearClamp.SetFilterMode(FILTER_LINEAR);        
              pTexStateLinearClamp.SetClampMode(true, true, true);
              int nTexStateLinearClampID = CTexture::GetTexState( pTexStateLinearClamp );

              pTerrainLightMap->Apply( -1, nTexStateLinearClampID, nTexSlot, nSamplerSlot );
            }
            else
            {
              CTexture::s_ptexWhite->Apply( -1 );
            }

            break;
          }

        case TO_BACKBUFFERSCALED_D2:
          CTexture::s_ptexBackBufferScaled[0]->Apply( -1 , nTS, nTexSlot, nSamplerSlot);
          break;
        case TO_BACKBUFFERSCALED_D4:
          CTexture::s_ptexBackBufferScaled[1]->Apply( -1 , nTS, nTexSlot, nSamplerSlot);
          break;
        case TO_BACKBUFFERSCALED_D8:
          CTexture::s_ptexBackBufferScaled[2]->Apply( -1, nTS, nTexSlot, nSamplerSlot);
          break;

        case TO_CLOUDS_LM:
          {

            // do more funky stuff
            static ICVar* e_shadows( gEnv->pConsole->GetCVar( "e_Shadows" ) );
            static ICVar* e_shadows_clouds( gEnv->pConsole->GetCVar( "e_ShadowsClouds" ) );
            bool setupCloudShadows( e_shadows->GetIVal() > 0 && e_shadows_clouds->GetIVal() > 0 );

            if( setupCloudShadows )
            {
              // cloud shadow map 
              CTexture* pCloudShadowTex( rd->GetCloudShadowTextureId() > 0 ? CTexture::GetByID( rd->GetCloudShadowTextureId() ) : CTexture::s_ptexWhite );
              assert( pCloudShadowTex );

              STexState pTexStateLinearClamp;
              pTexStateLinearClamp.SetFilterMode(FILTER_LINEAR);        
              pTexStateLinearClamp.SetClampMode(false, false, false);
              int nTexStateLinearClampID = CTexture::GetTexState( pTexStateLinearClamp );

              pCloudShadowTex->Apply( -1, nTexStateLinearClampID, nTexSlot, nSamplerSlot );
            }
            else
            {
              CTexture::s_ptexWhite->Apply( -1 );
            }

            break;
          }

        case TO_MIPCOLORS_DIFFUSE:
          {
            CTexture *pTex = NULL;
            if (pSR && pSR->m_Textures[EFTT_DIFFUSE])
            {
              CTexture *tex = pSR->m_Textures[EFTT_DIFFUSE]->m_Sampler.m_pTex;
              pTex = CTexture::GenerateMipsColorMap(tex->GetWidth(), tex->GetHeight());
              if (pTex)
                pTex->Apply(-1);
            }
            if (!pTex)
              CTexture::s_ptexWhite->Apply(-1);
          }
          break;

        case TO_MIPCOLORS_BUMP:
          {
            CTexture *pTex = NULL;
            if (pSR && pSR->m_Textures[EFTT_BUMP])
            {
              CTexture *tex = pSR->m_Textures[EFTT_BUMP]->m_Sampler.m_pTex;
              pTex = CTexture::GenerateMipsColorMap(tex->GetWidth(), tex->GetHeight());
              if (pTex)
                pTex->Apply(-1);
            }
            if (!pTex)
              CTexture::s_ptexWhite->Apply(-1);
          }
          break;

        case TO_BACKBUFFERMAP:        
          CTexture::s_ptexBackBuffer->Apply(-1, nTS);    
          break;
        case TO_HDRTARGET_ENCODED:






          break;

#ifndef EXCLUDE_SCALEFORM_SDK
        case TO_FROMSF0:
        case TO_FROMSF1:
          {
            const SSF_GlobalDrawParams* pParams(rd->SF_GetGlobalDrawParams());
            assert(pParams);
            int texIdx(nCustomID - TO_FROMSF0);
            int texID(pParams ? pParams->texture[texIdx].texID : -1);
            if (texID > 0)
            {
              const static int texStateID[8] = 
              {
                CTexture::GetTexState(STexState(FILTER_POINT, false)), CTexture::GetTexState(STexState(FILTER_POINT, true)),
                CTexture::GetTexState(STexState(FILTER_LINEAR, false)), CTexture::GetTexState(STexState(FILTER_LINEAR, true)),
                CTexture::GetTexState(STexState(FILTER_TRILINEAR, false)), CTexture::GetTexState(STexState(FILTER_TRILINEAR, true)),
                -1, -1
              };
              CTexture* pTex(CTexture::GetByID(texID));
              int textStateID(texStateID[pParams->texture[texIdx].texState]);
              pTex->Apply(-1, textStateID);
            }
            else
            {
              CTexture::s_ptexWhite->Apply(-1);
            }
            break;
          }
#endif

			case TO_VOLOBJ_DENSITY:
			case TO_VOLOBJ_SHADOW:
				{
					bool texBound(false);
					CRendElementBase* pRE(rd->m_RP.m_pRE);
					if (pRE && pRE->mfGetType() == eDATA_VolumeObject)
					{
						CREVolumeObject* pVolObj((CREVolumeObject*)pRE);
						int texId(0);
						if (pVolObj)
						{
							switch(nCustomID)
							{
							case TO_VOLOBJ_DENSITY:
								if (pVolObj->m_pDensVol)
									texId = pVolObj->m_pDensVol->GetTexID();
								break;
							case TO_VOLOBJ_SHADOW:
								if (pVolObj->m_pShadVol)
									texId = pVolObj->m_pShadVol->GetTexID();
								break;
							default:
								assert(0);
								break;
							}
						}
						CTexture* pTex(texId > 0 ? CTexture::GetByID(texId) : 0);
						if (pTex)
						{
							pTex->Apply(-1, nTS, nTexSlot, nSamplerSlot);
							texBound = true;
						}
					}
					if (!texBound)
						CTexture::s_ptexWhite->Apply(-1);
					break;
				}

			case TO_COLORCHART:
				{
					CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D;
					if (pCtrl)
					{
						CTexture* pTex = pCtrl->GetColorChart();
						if (pTex)
						{
							const static int texStateID = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
							pTex->Apply(-1, texStateID);
							break;
						}
					}

					gRenDev->CV_r_colorgrading_charts = 0;
					CTexture::s_ptexWhite->Apply(-1);
					break;
				}

			case TO_SKYDOME_MIE:
			case TO_SKYDOME_RAYLEIGH:
				{
					CRendElementBase* pRE = rd->m_RP.m_pRE;
					if (pRE && pRE->mfGetType() == eDATA_HDRSky)
					{
						CTexture* pTex = nCustomID == TO_SKYDOME_MIE ? ((CREHDRSky*) pRE)->m_pSkyDomeTextureMie : ((CREHDRSky*) pRE)->m_pSkyDomeTextureRayleigh;
						if (pTex)
						{
							pTex->Apply();
							break;
						}
					}
					CTexture::s_ptexBlack->Apply();
					break;
				}

			case TO_SKYDOME_MOON:
				{
					CRendElementBase* pRE = rd->m_RP.m_pRE;
					if (pRE && pRE->mfGetType() == eDATA_HDRSky)
					{
						CREHDRSky* pHDRSky = (CREHDRSky*) pRE;
						CTexture* pMoonTex(pHDRSky->m_moonTexId > 0 ? CTexture::GetByID(pHDRSky->m_moonTexId) : 0);
						if (pMoonTex)
						{
							const static int texStateID = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
							pMoonTex->Apply(-1, texStateID);
							break;
						}
					}
					CTexture::s_ptexBlack->Apply();
					break;
				}

			case TO_IRRADVOLUME_R:
				if(CTexture::s_ptexIrrVolumeRT[0])
					CTexture::s_ptexIrrVolumeRT[0]->Apply(-1, nTS, nTexSlot, nSamplerSlot);    
				break;
			case TO_IRRADVOLUME_G:
				if(CTexture::s_ptexIrrVolumeRT[1])
					CTexture::s_ptexIrrVolumeRT[1]->Apply(-1, nTS, nTexSlot, nSamplerSlot);    
				break;
			case TO_IRRADVOLUME_B:
				if(CTexture::s_ptexIrrVolumeRT[2])
					CTexture::s_ptexIrrVolumeRT[2]->Apply(-1, nTS, nTexSlot, nSamplerSlot);    
				break;
			case TO_IRRADVOLUME_NORMAL:
				if(CTexture::s_ptexIrrVolumeNormalMap)
					CTexture::s_ptexIrrVolumeNormalMap->Apply(-1, nTS, nTexSlot, nSamplerSlot);    
				break;
			case TO_IRRADVOLUME_COLOR:
				if(CTexture::s_ptexIrrVolumeColorMap)
					CTexture::s_ptexIrrVolumeColorMap->Apply(-1, nTS, nTexSlot, nSamplerSlot);    
				break;
			case TO_IRRADVOLUME_DEPTH:
				if(CTexture::s_ptexIrrVolumeDepthMap)
					CTexture::s_ptexIrrVolumeDepthMap->Apply(-1, nTS, nTexSlot, nSamplerSlot);    
				break;
      default:
        {
          tx->Apply(-1, nTS, nTexSlot, nSamplerSlot);
        }
        break;
      }
    }
  }

  return true;
}

bool CHWShader_D3D::mfUpdateSamplers()
{
  if (!m_pCurInst)
    return false;
  SHWSInstance *pInst = m_pCurInst;
  if (!pInst->m_pSamplers.size())
    return true;

  CD3D9Renderer *rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
  
  STexSampler *pSamp = &pInst->m_pSamplers[0];

  SRenderShaderResources *pSR = gRenDev->m_RP.m_pShaderResources;	

  uint32 i;
  const uint32 nSize = pInst->m_pSamplers.size();
  bool bDiffuseSlotUpdated = false;
	
  for (i=0; i<nSize; i++, pSamp++)
  {
    CTexture *tx = pSamp->m_pTex;
    //assert(tx);
    if (!tx)
      continue;
    if (tx >= &CTexture::s_ShaderTemplates[0] && tx <= &CTexture::s_ShaderTemplates[EFTT_MAX-1])
    {
      //if (tx < &CTexture::s_ShaderTemplates[EFTT_LIGHTMAP] || tx > &CTexture::s_ShaderTemplates[EFTT_OCCLUSION])
      {
        int nSlot = (int)(tx - &CTexture::s_ShaderTemplates[0]);
        if (pSR && pSR->m_Textures[nSlot])
        {



					if( nSlot != EFTT_DETAIL_OVERLAY || (nSlot == EFTT_DETAIL_OVERLAY && !(gRenDev->m_RP.m_pShader->m_Flags2 & EF2_DETAILBUMPMAPPING)))
						pSR->m_Textures[nSlot]->Update(nSlot);

          if( nSlot == EFTT_DIFFUSE ) 
            bDiffuseSlotUpdated = true;
          
          if ( (rTI.m_PersFlags & RBPF_ZPASS) && nSlot == EFTT_BUMP && pSR && !bDiffuseSlotUpdated) 
          {



            if (pSR->m_Textures[EFTT_DIFFUSE] )
              pSR->m_Textures[EFTT_DIFFUSE]->Update(EFTT_DIFFUSE);
          }


          tx = pSR->m_Textures[nSlot]->m_Sampler.m_pTex;
        }
        else
        {
          if (nSlot == EFTT_BUMP)
            tx = CTexture::s_ptexFlatBump;
        }
      }
    }
  }

  return true;
}

void CHWShader_D3D::mfInit()
{
}

ED3DShError CHWShader_D3D::mfFallBack(SHWSInstance *&pInst, int nStatus)
{
  // No fallback for:
  // 1. ShadowGen pass
  // 2. Z-prepass
  // 3. Glow-pass
  // 4. Shadow-pass
#if defined (DIRECT3D9) && !defined(XENON) && !defined(PS3)
  if (CParserBin::m_bPS3 || CParserBin::m_bXenon || CParserBin::m_bD3D11)
  {
    assert(gRenDev->m_cEF.m_nCombinationsProcess >= 0);
    return ED3DShError_CompilingError;
  }
#endif



  if (m_eSHClass == eHWSC_Geometry || (gRenDev->m_RP.m_nBatchFilter & (FB_ZPREPASS | FB_Z | FB_GLOW | FB_SCATTER)) || (gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN) || gRenDev->m_RP.m_nPassGroupID == EFSLIST_SHADOW_PASS)
    return ED3DShError_CompilingError;
  if (gRenDev->m_RP.m_pShader)
  {
    if (gRenDev->m_RP.m_pShader->GetShaderType() == eST_HDR || gRenDev->m_RP.m_pShader->GetShaderType() == eST_PostProcess || gRenDev->m_RP.m_pShader->GetShaderType() == eST_Water || gRenDev->m_RP.m_pShader->GetShaderType() == eST_Shadow || gRenDev->m_RP.m_pShader->GetShaderType() == eST_Shadow)
      return ED3DShError_CompilingError;
  }
  // Skip rendering if async compiling Cvar is 2
  if (CRenderer::CV_r_shadersasynccompiling == 2)
    return ED3DShError_CompilingError;

  CShader *pSH = CShaderMan::m_ShaderFallback;
  int nTech = 0;
  if (nStatus == -1)
  {
    pInst->m_Handle.m_bStatus = 1;
    nTech = 1;
  }
  else
  {
    nTech = 0;
    assert(nStatus == 0);
  }
  if (gRenDev->m_RP.m_pShader && gRenDev->m_RP.m_pShader->GetShaderType() == eST_Terrain)
    nTech += 2;

  assert(pSH);
  if (CRenderer::CV_r_logShaders)
  {
    char nameSrc[256];
    mfGetDstFileName(pInst, this, nameSrc, 256, 3);
    gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: using Fallback tech '%s' instead of 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pSH->m_HWTechniques[nTech]->m_Name.c_str(), pInst, nameSrc);
  }
  // Fallback
  if (pSH)
  {
    if (gRenDev->m_RP.m_CurState & GS_DEPTHFUNC_EQUAL)
    {
      int nState = gRenDev->m_RP.m_CurState & ~GS_DEPTHFUNC_EQUAL;
      nState |= GS_DEPTHWRITE;
      gRenDev->EF_SetState(nState);
    }
    CHWShader_D3D *pHWSH;
    if (m_eSHClass == eHWSC_Vertex)
    {
      pHWSH = (CHWShader_D3D *)pSH->m_HWTechniques[nTech]->m_Passes[0].m_VShader;
#ifdef DO_RENDERLOG
      if (CRenderer::CV_r_log >= 3)
        gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "---- Fallback FX VShader \"%s\"\n", pHWSH->GetName());
#endif
    }
    else
    {
      pHWSH = (CHWShader_D3D *)pSH->m_HWTechniques[nTech]->m_Passes[0].m_PShader;
#ifdef DO_RENDERLOG
      if (CRenderer::CV_r_log >= 3)
        gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "---- Fallback FX PShader \"%s\"\n", pHWSH->GetName());
#endif
    }

    if (!pHWSH->m_Insts.size())
    {
      SShaderCombination cmb;
      pHWSH->mfPrecache(cmb, true);
    }
    if (pHWSH->m_Insts.size())
    {
      SHWSInstance *pInstF = &pHWSH->m_Insts[0];
      if (!pInstF->m_Handle.m_pShader || !pInstF->m_Handle.m_pShader->m_pHandle)
        return ED3DShError_CompilingError;
      pInst = pInstF;
      m_pCurInst = pInstF;
      pInstF->m_bFallback = true;
    }
    else
      return ED3DShError_CompilingError;
  }
  //if (nStatus == 0)
  //  return ED3DShError_Compiling;
  return ED3DShError_Ok;

}

ED3DShError CHWShader_D3D::mfIsValid(SHWSInstance *&pInst, bool bFinalise)
{
  //if (stricmp(m_EntryFunc.c_str(), "FPPS") && stricmp(m_EntryFunc.c_str(), "FPVS") && stricmp(m_EntryFunc.c_str(), "AuxGeomPS") && stricmp(m_EntryFunc.c_str(), "AuxGeomVS"))
  //  return mfFallBack(pInst, 0);

  if (pInst->m_Handle.m_pShader)
    return ED3DShError_Ok;
  if (pInst->m_Handle.m_bStatus == 1)
  {
    return mfFallBack(pInst, -1);
  }
  if (pInst->m_Handle.m_bStatus == 2)
    return ED3DShError_Fake;
  if (pInst->m_Handle.m_pShader == NULL)
  {
#if defined (WIN32) || defined(XENON)
    if (!bFinalise || !pInst->m_pAsync)
      return ED3DShError_NotCompiled;
    int nStatus = mfAsyncCompileReady(pInst);
    if (nStatus == 1)
    {
      if (gcpRendD3D->m_cEF.m_nCombinationsProcess<=0 || gcpRendD3D->m_cEF.m_bActivatePhase)
      {
        assert(pInst->m_Handle.m_pShader != NULL);
      }
      return ED3DShError_Ok;
    }
    return mfFallBack(pInst, nStatus);
#else
    return ED3DShError_NotCompiled;
#endif
  }
  return ED3DShError_Ok;
}

//========================================================================================

void CHWShader::mfBeginFrame(int nMaxToFlush)
{
#if !defined (XENON) && defined (WIN32)
  //CHWShader_D3D::mfFlushPendedShaders(nMaxToFlush);
#endif
}

void CHWShader::mfLazyUnload()
{
  int nScanned = 0;
  int nUnloaded = 0;
  static int nLastScannedPS = 0;
  static int nLastScannedVS = 0;
  static int sbReset = 0;
  if (!gRenDev->m_bEnd)
  {
    sbReset = 0;
    nLastScannedPS = 0;
    nLastScannedVS = 0;
    return;
  }
  CCryName Name = CHWShader::mfGetClassName(eHWSC_Pixel);
  SResourceContainer *pRL = CBaseResource::GetResourcesForClass(Name);
  uint32 i;
	int j;
  float fTime = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;

  float fThr = (float)CRenderer::CV_r_shaderslazyunload;
  if (pRL)
  {
    for (i=nLastScannedPS; i<pRL->m_RList.size(); i++)
    {
      CHWShader_D3D *pPS = (CHWShader_D3D *)pRL->m_RList[i];
      int nDeleted = 0;
      for (j=0; j<pPS->m_Insts.size(); j++)
      {
        CHWShader_D3D::SHWSInstance *pInst = &pPS->m_Insts[j];
        if (pInst->m_bDeleted)
          continue;
        if (pInst->m_pAsync)
          continue;
        if (sbReset != 3)
          pInst->m_fLastAccess = fTime;
        else
        if (fTime-pInst->m_fLastAccess > fThr)
        {
          pPS->m_pCurInst = pInst;
          pInst->Release(pPS->m_pCache, false);
          pInst->m_bDeleted = true;
          nDeleted++;
          nUnloaded++;
          pPS->m_pCurInst = NULL;
        }
      }
      //if (nDeleted == pPS->m_Insts.size())
      //  pPS->mfReset(0);
      nScanned++;
      if (nUnloaded > 16)
        break;
      if (nScanned > 32)
        break;
    }
    if (i >= pRL->m_RList.size())
    {
      sbReset |= 1;
      i = 0;
    }
    nLastScannedPS = i;
  }
  Name = CHWShader::mfGetClassName(eHWSC_Vertex);
  pRL = CBaseResource::GetResourcesForClass(Name);
  nUnloaded = 0;
  nScanned = 0;
  if (pRL)
  {
    for (i=nLastScannedVS; i<pRL->m_RList.size(); i++)
    {
      CHWShader_D3D *pVS = (CHWShader_D3D *)pRL->m_RList[i];
      int nDeleted = 0;
      for (j=0; j<pVS->m_Insts.size(); j++)
      {
        CHWShader_D3D::SHWSInstance *pInst = &pVS->m_Insts[j];
        if (pInst->m_bDeleted)
          continue;
        if (pInst->m_pAsync)
          continue;
        if (sbReset != 3)
          pInst->m_fLastAccess = fTime;
        else
        if (fTime-pInst->m_fLastAccess > CRenderer::CV_r_shaderslazyunload)
        {
          pVS->m_pCurInst = pInst;
          pInst->Release(pVS->m_pCache, false);
          pInst->m_bDeleted = true;
          nDeleted++;
          nUnloaded++;
          pVS->m_pCurInst = NULL;
        }
      }
      //if (nDeleted == pVS->m_Insts.size())
      //  pVS->mfReset(0);
      nScanned++;
      if (nUnloaded > 16)
        break;
      if (nScanned > 32)
        break;
    }
    if (i >= pRL->m_RList.size())
    {
      sbReset |= 2;
      i = 0;
    }
    nLastScannedVS = i;
  }
}

CHWShader_D3D::SHWSInstance *CHWShader_D3D::mfGetInstance(int nInstance, uint64 GLMask)
{
  DynArray<SHWSInstance> *pInstCont = &m_Insts;
  if (m_Flags & HWSG_SHARED)
  {
    pInstCont = mfGetSharedInstContainer(true, GLMask, false);
    assert(pInstCont);
  }
  return &(*pInstCont)[nInstance];
}

void CHWShader_D3D::mfSetForOverdraw(SHWSInstance *pInst, uint32 nFlags, uint64& RTMask)
{
	if (mfIsValid(pInst, false) ==  ED3DShError_NotCompiled)
		mfActivate(nFlags);
	RTMask |= g_HWSR_MaskBit[HWSR_DEBUG0] | g_HWSR_MaskBit[HWSR_DEBUG1] | g_HWSR_MaskBit[HWSR_DEBUG2] | g_HWSR_MaskBit[HWSR_DEBUG3];
	RTMask &= m_nMaskAnd_RT;
  RTMask |= m_nMaskOr_RT;
	CD3D9Renderer *rd = gcpRendD3D;
	if (CRenderer::CV_r_measureoverdraw == 1 && m_eSHClass == eHWSC_Pixel)
		rd->m_RP.m_NumShaderInstructions = pInst->m_nInstructions;
	else
	if (CRenderer::CV_r_measureoverdraw == 3 && m_eSHClass == eHWSC_Vertex)
		rd->m_RP.m_NumShaderInstructions = pInst->m_nInstructions;
	else
	if (CRenderer::CV_r_measureoverdraw == 2)
		rd->m_RP.m_NumShaderInstructions = 30;
}

CHWShader_D3D::SHWSInstance *CHWShader_D3D::mfGetInstance(uint64 RTMask, uint32 LightMask, uint64 GLMask, uint32 MDMask, uint32 MDVMask, uint32 nFlags)
{
  SHWSInstance *cgi;
  if ((m_Flags & HWSG_SHARED) && m_nInstFrame != m_nCurInstFrame)
  {
    m_nCurInstFrame = m_nInstFrame;
    m_pCurInst = NULL;
  }
  if (m_pCurInst && !m_pCurInst->m_bFallback)
  {
    cgi = m_pCurInst;
    assert(cgi->m_eClass < eHWSC_Max);
    if (cgi->m_RTMask == RTMask && cgi->m_GLMask == GLMask && cgi->m_LightMask == LightMask && cgi->m_MDMask == MDMask && cgi->m_MDVMask == MDVMask)
    {
      if ((CParserBin::m_bPS3 && cgi->m_bPS3) || (CParserBin::m_bXenon == cgi->m_bXenon && CParserBin::m_bD3D11 == cgi->m_bD3D11))
        return cgi;
    }
  } 
  uint32 i;
  DynArray<SHWSInstance> *pInstCont = &m_Insts;
  if (m_Flags & HWSG_SHARED)
    pInstCont = mfGetSharedInstContainer(true, GLMask, (nFlags & HWSF_PRECACHE_INST)!=0);
  //int nFree = -1;
  for (i=0; i<(uint32)pInstCont->size(); i++)
  {
    cgi = &(*pInstCont)[i];
    /*if (cgi->m_eProfileType == eHWSP_Deleted)
    {
      nFree = i;
      continue;
    }*/
    if (cgi->m_RTMask == RTMask && cgi->m_GLMask == GLMask && cgi->m_LightMask == LightMask && cgi->m_MDMask == MDMask && cgi->m_MDVMask == MDVMask)
    {
      if ((CParserBin::m_bPS3 && cgi->m_bPS3) || (CParserBin::m_bXenon == cgi->m_bXenon && CParserBin::m_bD3D11 == cgi->m_bD3D11))
      {
        m_pCurInst = cgi;
        return cgi;
      }
    }
  }
  SHWSInstance ci;
  ci.m_nVertexFormat = 1;
  ci.m_nCache = -1;
  m_nInstFrame++;
  ci.m_RTMask = RTMask;
  ci.m_GLMask = GLMask;
  ci.m_LightMask = LightMask;
  ci.m_MDMask = MDMask;
  ci.m_MDVMask = MDVMask;
  ci.m_bXenon = CParserBin::m_bXenon;
  ci.m_bPS3 = CParserBin::m_bPS3;
  ci.m_bD3D11 = CParserBin::m_bD3D11;
  ci.m_eClass = m_eSHClass;
  /*if (nFree >= 0)
  {
    (*pInstCont)[nFree] = ci;
    m_pCurInst = &(*pInstCont)[nFree];
  }
  else*/
  {
    pInstCont->push_back(ci);
    m_pCurInst = &(*pInstCont)[i];
  }
  if (nFlags & HWSF_FAKE)
  {
    m_pCurInst->m_Handle.SetFake();
    //mfSetHWStartProfile(nFlags);
  }
  return m_pCurInst;
}

//=================================================================================

void CHWShader_D3D::ModifyLTMask(uint32& nMask)
{
  if (nMask)
  {
    if (!(m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING | HWSG_FP_EMULATION)))
      nMask = 0;
    else
    if (!(m_Flags & HWSG_SUPPORTS_MULTILIGHTS) && (m_Flags & HWSG_SUPPORTS_LIGHTING))
    {
      int nLightType = (nMask >> SLMF_LTYPE_SHIFT) & SLMF_TYPE_MASK;
      if (nLightType != SLMF_PROJECTED)
        nMask = 1;
    }
  }
}

bool CHWShader_D3D::mfSetVS(int nFlags)
{
  PROFILE_FRAME(Shader_SetShadersVS);





  CD3D9Renderer *rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  uint32 LTMask = rRP.m_FlagsShader_LT;
  uint64 RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
  uint32 MDMask = rRP.m_FlagsShader_MD;
  uint32 MDVMask = rRP.m_FlagsShader_MDV;
  /*if (RTMask == 0x40004 && !stricmp(m_EntryFunc.c_str(), "Common_ZPassVS"))
  {
    int nnn = 0;
  }*/

  ModifyLTMask(LTMask);

  SHWSInstance *pInst = mfGetInstance(RTMask, LTMask, m_nMaskGenShader, MDMask, MDVMask, nFlags); 

  if (CRenderer::CV_r_measureoverdraw == 3)
  {
    mfSetForOverdraw(pInst, nFlags, RTMask);
    pInst = mfGetInstance(RTMask, LTMask, m_nMaskGenShader, MDMask, MDVMask, nFlags);
  }
  
  pInst->m_fLastAccess = rTI.m_RealTime;

  if (!mfCheckActivation(pInst, nFlags))
  {
#if defined(DIRECT3D10) || defined(XENON)
    m_pCurInstVS = NULL;
#endif
    return false;
  }

#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log >= 3)
  {
#if defined(__GNUC__)
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX VShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x\n", GetName(), pInst->m_nInstructions, pInst->m_LightMask, m_nMaskGenShader, RTMask, MDMask, MDVMask);
#else
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX VShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%I64x, RTMask: 0x%I64x, MDMask: 0x%x, MDVMask: 0x%x\n", GetName(), pInst->m_nInstructions, pInst->m_LightMask, m_nMaskGenShader, RTMask, MDMask, MDVMask);
#endif
  }
#endif
  if (m_nFrame != rTI.m_nFrameUpdateID)
  {
    m_nFrame = rTI.m_nFrameUpdateID;
    rRP.m_PS[rRP.m_nProcessThreadID].m_NumVShaders++;
    if (pInst->m_nInstructions > rRP.m_PS[rRP.m_nProcessThreadID].m_NumVSInstructions)
    {
      rRP.m_PS[rRP.m_nProcessThreadID].m_NumVSInstructions = pInst->m_nInstructions;
      rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxVShader = this;
      rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxVSInstance = pInst;
    }
  }
  if (!(nFlags & HWSF_PRECACHE))
  {
    if (m_pCurVS != pInst->m_Handle.m_pShader)
    {
      m_pCurVS = pInst->m_Handle.m_pShader;
      rRP.m_PS[rRP.m_nProcessThreadID].m_NumVShadChanges++;
      mfBind();
    }
#if defined(DIRECT3D10) || defined(XENON)
    m_pCurInstVS = pInst;
#endif
    if (rRP.m_pRE)
      rRP.m_CurVFormat = (EVertexFormat)pInst->m_nVertexFormat;
    rRP.m_FlagsStreams_Decl = pInst->m_VStreamMask_Decl;
    rRP.m_FlagsStreams_Stream = pInst->m_VStreamMask_Stream;
    // Make sure we don't use any texture attributes except baseTC in instancing case
    if (nFlags & HWSF_INSTANCED)
    {
      rRP.m_FlagsStreams_Decl &= ~(VSM_MORPHBUDDY);
      rRP.m_FlagsStreams_Stream &= ~(VSM_MORPHBUDDY);
    }

    mfSetParametersPB();
  }

  return true;
}

bool CHWShader_D3D::mfSetPS(int nFlags)
{
  PROFILE_FRAME(Shader_SetShadersPS);





  CD3D9Renderer *rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
























  uint32 LTMask = rRP.m_FlagsShader_LT;
  uint64 RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
  uint32 MDMask = rRP.m_FlagsShader_MD & ~HWMD_TCMASK;
  uint32 MDVMask = 0;

  if (LTMask)
  {
    if (!(m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING | HWSG_FP_EMULATION)))
      LTMask = 0;
    else
    if (!(m_Flags & HWSG_SUPPORTS_MULTILIGHTS) && (m_Flags & HWSG_SUPPORTS_LIGHTING))
    {
      int nLightType = (LTMask >> SLMF_LTYPE_SHIFT) & SLMF_TYPE_MASK;
      if (nLightType != SLMF_PROJECTED)
        LTMask = 1;
    }
  }

  SHWSInstance *pInst = mfGetInstance(RTMask, LTMask, m_nMaskGenShader, MDMask, MDVMask, nFlags);

  // Update texture modificator flags based on active samplers state
  if (nFlags & HWSF_SETTEXTURES)
  {
    int nResult = mfCheckActivation(pInst, nFlags);
    if (!nResult)
    {
#if defined(DIRECT3D10) || defined(XENON)
      CHWShader_D3D::m_pCurInstPS = NULL;
#endif
      return false;
    }
    mfUpdateSamplers();
    if ((rRP.m_FlagsShader_MD ^ MDMask) & ~HWMD_TCMASK)
    {
      pInst->m_fLastAccess = rTI.m_RealTime;
      MDMask = rRP.m_FlagsShader_MD & ~HWMD_TCMASK;
      pInst = mfGetInstance(RTMask, LTMask, m_nMaskGenShader, MDMask, MDVMask, nFlags);
    }
  }
  if (CRenderer::CV_r_measureoverdraw>0 && CRenderer::CV_r_measureoverdraw<4)
  {
    mfSetForOverdraw(pInst, nFlags, RTMask);
    pInst = mfGetInstance(RTMask, LTMask, m_nMaskGenShader, MDMask, MDVMask, nFlags);
  }
  pInst->m_fLastAccess = rTI.m_RealTime;

  if (!mfCheckActivation(pInst, nFlags))
  {
#if defined(DIRECT3D10) || defined(XENON)
    m_pCurInstPS = NULL;
#endif
    return false;
  }

#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log >= 3)
  {
#if defined(__GNUC__)
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX PShader \"%s\" (%d instr)LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x\n", GetName(), pInst->m_nInstructions, pInst->m_LightMask, m_nMaskGenShader, RTMask, MDMask, MDVMask);
#else
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX PShader \"%s\" (%d instr) LTMask: 0x%x, GLMask: 0x%I64x, RTMask: 0x%I64x, MDMask: 0x%x, MDVMask: 0x%x\n", GetName(), pInst->m_nInstructions, pInst->m_LightMask, m_nMaskGenShader, RTMask, MDMask, MDVMask);
#endif
  }
#endif

  if (m_nFrame != rTI.m_nFrameUpdateID)
  {
    m_nFrame = rTI.m_nFrameUpdateID;
    rRP.m_PS[rRP.m_nProcessThreadID].m_NumPShaders++;
    if (pInst->m_nInstructions > rRP.m_PS[rRP.m_nProcessThreadID].m_NumPSInstructions)
    {
      rRP.m_PS[rRP.m_nProcessThreadID].m_NumPSInstructions = pInst->m_nInstructions;
      rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxPShader = this;
      rRP.m_PS[rRP.m_nProcessThreadID].m_pMaxPSInstance = pInst;
    }
  }
  if (!(nFlags & HWSF_PRECACHE))
  {
    if (m_pCurPS != pInst->m_Handle.m_pShader)
    {
      m_pCurPS = pInst->m_Handle.m_pShader;
      rRP.m_PS[rRP.m_nProcessThreadID].m_NumPShadChanges++;
      mfBind();
    }
#if defined(DIRECT3D10) || defined(XENON)
    m_pCurInstPS = pInst;
#endif
    mfSetParametersPB();
    if (nFlags & HWSF_SETTEXTURES)
      mfSetSamplers();
  }

  return true;
}

#if defined(DIRECT3D10)
bool CHWShader_D3D::mfSetGS(int nFlags)
{
  PROFILE_FRAME(Shader_SetShadersGS);

  CD3D9Renderer *rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  uint32 LTMask = rRP.m_FlagsShader_LT;
  uint64 RTMask = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
  uint32 MDMask = rRP.m_FlagsShader_MD;
  uint32 MDVMask = rRP.m_FlagsShader_MDV;

  if (LTMask)
  {
    if (!(m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING | HWSG_FP_EMULATION)))
      LTMask = 0;
    else
    if (!(m_Flags & HWSG_SUPPORTS_MULTILIGHTS) && (m_Flags & HWSG_SUPPORTS_LIGHTING))
    {
      int nLightType = (LTMask >> SLMF_LTYPE_SHIFT) & SLMF_TYPE_MASK;
      if (nLightType != SLMF_PROJECTED)
        LTMask = 1;
    }
  }

  SHWSInstance *pInst = mfGetInstance(RTMask, LTMask, m_nMaskGenShader, MDMask, MDVMask, nFlags);
  pInst->m_fLastAccess = rTI.m_RealTime;

  if (!mfCheckActivation(pInst, nFlags))
  {
#if defined(DIRECT3D10) || defined(XENON)
    m_pCurInstGS = NULL;
#endif
    return false;
  }

#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log >= 3)
  {
  #if defined(__GNUC__)
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX GShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%llx, RTMask: 0x%llx, MDMask: 0x%x, MDVMask: 0x%x\n", GetName(), pInst->m_nInstructions, pInst->m_LightMask, m_nMaskGenShader, RTMask, MDMask, MDVMask);
  #else
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Set FX GShader \"%s\" (%d instr), LTMask: 0x%x, GLMask: 0x%I64x, RTMask: 0x%I64x, MDMask: 0x%x, MDVMask: 0x%x\n", GetName(), pInst->m_nInstructions, pInst->m_LightMask, m_nMaskGenShader, RTMask, MDMask, MDVMask);
  #endif
  }
#endif

  m_pCurInstGS = pInst;
  if (!(nFlags & HWSF_PRECACHE))
  {
    mfBindGS(pInst->m_Handle.m_pShader, pInst->m_Handle.m_pShader->m_pHandle);

    mfSetParametersPB();
  }

  return true;
}
#endif

//=======================================================================

/* returns a random floating point number between 0.0 and 1.0 */
static float frand()
{
  return (float) (cry_rand() / (float) RAND_MAX);
}

/* returns a random floating point number between -1.0 and 1.0 */
static float sfrand()
{
  return (float) (cry_rand() * 2.0f/ (float) RAND_MAX) - 1.0f;
}

void CHWShader_D3D::mfSetLightParams(int nPass)
{
  CD3D9Renderer *rd = gcpRendD3D;
  uint32 i;

  SRenderShaderResources *pRes = rd->m_RP.m_pShaderResources;
  SLightPass *pLP = &rd->m_RP.m_LPasses[nPass];
  Vec3 vViewPos = rd->GetRCamera().Orig;

#ifndef XENON
  int nMaxLights = rd->m_RP.m_nMaxLightsPerPass;
  Vec4 *pDstPS = NULL;
  Vec4 *pDstVS = NULL;
  if (rd->m_RP.m_nShaderQuality == eSQ_Low && pRes && pRes->m_Constants[eHWSC_Pixel].size() >= 2)
  for (i=0; i<pLP->nLights; i++)
  {
    CDLight *pDL = pLP->pLights[i];
    sData[0].f[0] = pDL->m_Color[0] * pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][0];
    sData[0].f[1] = pDL->m_Color[1] * pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][1];
    sData[0].f[2] = pDL->m_Color[2] * pRes->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][2];
    sData[0].f[3] = 1;

    Vec3 v = pDL->m_Origin - vViewPos;
    sData[1].f[0] = v.x;
    sData[1].f[1] = v.y;
    sData[1].f[2] = v.z;
    float fRadius = pDL->m_fRadius;
    if (fRadius <= 0)
      fRadius = 1.f;
    sData[1].f[3] = 1.f / fRadius;

    sData[2].f[0] = pDL->m_ShadowChanMask.x;
    sData[2].f[1] = pDL->m_ShadowChanMask.y;
    sData[2].f[2] = pDL->m_ShadowChanMask.z;
    sData[2].f[3] = pDL->m_ShadowChanMask.w;

    sData[3].f[0] = pDL->m_Color[0] * pDL->m_SpecMult * pRes->m_Constants[eHWSC_Pixel][PS_SPECULAR_COL][0];
    sData[3].f[1] = pDL->m_Color[1] * pDL->m_SpecMult * pRes->m_Constants[eHWSC_Pixel][PS_SPECULAR_COL][1];
    sData[3].f[2] = pDL->m_Color[2] * pDL->m_SpecMult * pRes->m_Constants[eHWSC_Pixel][PS_SPECULAR_COL][2];
    sData[3].f[3] = 1.0f;

    if (sData[3].f[0]>0.1f || sData[3].f[1]>0.1f || sData[3].f[2]>0.1f)
      sData[3].f[3] = pRes->m_Constants[eHWSC_Pixel][PS_SPECULAR_COL][3];

 #ifndef DIRECT3D10
    if (CParserBin::m_bNewLightSetup)
    {
      mfParameterRegA(0, &sData[0].f[0], 4, eHWSC_Pixel);
      mfParameterRegA(6, &sData[1].f[0], 1, eHWSC_Vertex);
    }
    else
    {
      mfParameterRegA(0*nMaxLights+i, &sData[0].f[0], 1, eHWSC_Pixel);
      mfParameterRegA(1*nMaxLights+i, &sData[1].f[0], 1, eHWSC_Pixel);
      mfParameterRegA(2*nMaxLights+i, &sData[2].f[0], 1, eHWSC_Pixel);
      mfParameterRegA(3*nMaxLights+i, &sData[3].f[0], 1, eHWSC_Pixel);
      mfParameterRegA(6+i, &sData[1].f[0], 1, eHWSC_Vertex);
    }
 #else
    if (!m_pLightCB[eHWSC_Pixel])
    {
      D3D11_BUFFER_DESC bd;
      ZeroStruct(bd);
      HRESULT hr;

      bd.Usage = D3D11_USAGE_DYNAMIC;
      bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      bd.MiscFlags = 0;
      bd.ByteWidth = 4*4 * sizeof(Vec4);
      hr = gcpRendD3D->m_pd3dDevice->CreateBuffer(&bd, NULL, &m_pLightCB[eHWSC_Pixel]);
      assert(SUCCEEDED(hr));

      bd.ByteWidth = 4 * sizeof(Vec4);
      hr = gcpRendD3D->m_pd3dDevice->CreateBuffer(&bd, NULL, &m_pLightCB[eHWSC_Vertex]);
      assert(SUCCEEDED(hr));
    }
    if (!pDstPS)
		{
			STALL_PROFILER("set pixel_const_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pLightCB[eHWSC_Pixel], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			pDstPS = (Vec4*)mappedResource.pData;
		}
    Vec4 *pSrc = (Vec4 *)&sData[0].f[0];
    if (CParserBin::m_bNewLightSetup)
    {
      pDstPS[0] = pSrc[0];
      pDstPS[1] = pSrc[1];
      pDstPS[2] = pSrc[2];
      pDstPS[3] = pSrc[3];
    }
    else
    {
      pDstPS[0*nMaxLights+i] = pSrc[0];
      pDstPS[1*nMaxLights+i] = pSrc[1];
      pDstPS[2*nMaxLights+i] = pSrc[2];
      pDstPS[3*nMaxLights+i] = pSrc[3];
    }

    if (!pDstVS)
		{
			STALL_PROFILER("set vertex_const_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pLightCB[eHWSC_Vertex], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			pDstVS = (Vec4*)mappedResource.pData;
		}
    pDstVS[i] = pSrc[1];
 #endif
  }
  else
  for (i=0; i<pLP->nLights; i++)
  {
    CDLight *pDL = pLP->pLights[i];
    sData[0].f[0] = pDL->m_Color[0];
    sData[0].f[1] = pDL->m_Color[1];
    sData[0].f[2] = pDL->m_Color[2];
    sData[0].f[3] = pDL->m_SpecMult;

    Vec3 v = pDL->m_Origin - vViewPos;
    sData[1].f[0] = v.x;
    sData[1].f[1] = v.y;
    sData[1].f[2] = v.z;
    float fRadius = pDL->m_fRadius;
    if (fRadius <= 0)
      fRadius = 1.f;
    sData[1].f[3] = 1.f / fRadius;

    sData[2].f[0] = pDL->m_ShadowChanMask.x;
    sData[2].f[1] = pDL->m_ShadowChanMask.y;
    sData[2].f[2] = pDL->m_ShadowChanMask.z;
    sData[2].f[3] = pDL->m_ShadowChanMask.w;

 #ifndef DIRECT3D10
    if (CParserBin::m_bNewLightSetup)
    {
      mfParameterRegA(i*3, &sData[0].f[0], 3, eHWSC_Pixel);
      mfParameterRegA(6+i, &sData[1].f[0], 1, eHWSC_Vertex);
    }
    else
    {
      mfParameterRegA(0*nMaxLights+i, &sData[0].f[0], 1, eHWSC_Pixel);
      mfParameterRegA(1*nMaxLights+i, &sData[1].f[0], 1, eHWSC_Pixel);
      mfParameterRegA(2*nMaxLights+i, &sData[2].f[0], 1, eHWSC_Pixel);
      mfParameterRegA(6+i, &sData[1].f[0], 1, eHWSC_Vertex);
    }
 #else
    if (!m_pLightCB[eHWSC_Pixel])
    {
      D3D11_BUFFER_DESC bd;
      ZeroStruct(bd);
      HRESULT hr;

      bd.Usage = D3D11_USAGE_DYNAMIC;
      bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      bd.MiscFlags = 0;
      bd.ByteWidth = 3*4 * sizeof(Vec4);
      hr = gcpRendD3D->m_pd3dDevice->CreateBuffer(&bd, NULL, &m_pLightCB[eHWSC_Pixel]);
      assert(SUCCEEDED(hr));

      bd.ByteWidth = 4 * sizeof(Vec4);
      hr = gcpRendD3D->m_pd3dDevice->CreateBuffer(&bd, NULL, &m_pLightCB[eHWSC_Vertex]);
      assert(SUCCEEDED(hr));
    }
    if (!pDstPS)
		{
			STALL_PROFILER("set pixel_const_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pLightCB[eHWSC_Pixel], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			pDstPS = (Vec4*)mappedResource.pData;
		}
    Vec4 *pSrc = (Vec4 *)&sData[0].f[0];
    if (CParserBin::m_bNewLightSetup)
    {
      pDstPS[i*3+0] = pSrc[0];
      pDstPS[i*3+1] = pSrc[1];
      pDstPS[i*3+2] = pSrc[2];
    }
    else
    {
      pDstPS[0*nMaxLights+i] = pSrc[0];
      pDstPS[1*nMaxLights+i] = pSrc[1];
      pDstPS[2*nMaxLights+i] = pSrc[2];
    }
    if (!pDstVS)
		{
			STALL_PROFILER("set vert_const_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = gcpRendD3D->m_pd3dDeviceContext->Map(m_pLightCB[eHWSC_Vertex], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			pDstVS = (Vec4*)mappedResource.pData;
		}
    pDstVS[i] = pSrc[1];
 #endif
  }
 #ifdef DIRECT3D10
  if (pDstPS)
  {
		rd->GetDeviceContext()->Unmap(m_pLightCB[eHWSC_Pixel], 0);
    m_pCurReqCB[eHWSC_Pixel][CB_PER_LIGHT] = m_pLightCB[eHWSC_Pixel];
  }
  if (pDstVS)
  {
		rd->GetDeviceContext()->Unmap(m_pLightCB[eHWSC_Vertex], 0);
    m_pCurReqCB[eHWSC_Vertex][CB_PER_LIGHT] = m_pLightCB[eHWSC_Vertex];
  }
 #endif





















































#endif
}

void CHWShader_D3D::mfSetCM()
{
#if defined (DIRECT3D10)
  if (!m_CM_Params[eHWSC_Pixel].empty())
  {
    mfSetParameters(&m_CM_Params[eHWSC_Pixel][0], m_CM_Params[eHWSC_Pixel].size(), eHWSC_Pixel, m_nMax_PF_Vecs[eHWSC_Pixel]);
  }
  if (!m_CM_Params[eHWSC_Vertex].empty())
  {
    mfSetParameters(&m_CM_Params[eHWSC_Vertex][0], m_CM_Params[eHWSC_Vertex].size(), eHWSC_Vertex, m_nMax_PF_Vecs[eHWSC_Vertex]);
  }
#ifndef PS3
  if (!m_CM_Params[eHWSC_Geometry].empty())
  {
    mfSetParameters(&m_CM_Params[eHWSC_Geometry][0], m_CM_Params[eHWSC_Geometry].size(), eHWSC_Geometry, m_nMax_PF_Vecs[eHWSC_Geometry]);
  }
#endif
#else
  if (!m_CM_Params[eHWSC_Pixel].empty())
  {
    mfSetParameters(&m_CM_Params[eHWSC_Pixel][0], m_CM_Params[eHWSC_Pixel].size(), eHWSC_Pixel);
  }
  if (!m_CM_Params[eHWSC_Vertex].empty())
  {
    mfSetParameters(&m_CM_Params[eHWSC_Vertex][0], m_CM_Params[eHWSC_Vertex].size(), eHWSC_Vertex);
  }
  if (!m_CM_Params[eHWSC_Geometry].empty())
  {
    mfSetParameters(&m_CM_Params[eHWSC_Geometry][0], m_CM_Params[eHWSC_Geometry].size(), eHWSC_Geometry);
  }
#endif
}

void CHWShader_D3D::mfSetPF()
{
  CD3D9Renderer *r = gcpRendD3D;

#if defined (DIRECT3D10)
  if (!m_PF_Params[eHWSC_Pixel].empty())
  {
    mfSetParameters(&m_PF_Params[eHWSC_Pixel][0], m_PF_Params[eHWSC_Pixel].size(), eHWSC_Pixel, m_nMax_PF_Vecs[eHWSC_Pixel]);
  }
  if (!m_PF_Params[eHWSC_Vertex].empty())
  {
    mfSetParameters(&m_PF_Params[eHWSC_Vertex][0], m_PF_Params[eHWSC_Vertex].size(), eHWSC_Vertex, m_nMax_PF_Vecs[eHWSC_Vertex]);
  }
#ifndef PS3
  if (!m_PF_Params[eHWSC_Geometry].empty())
  {
    mfSetParameters(&m_PF_Params[eHWSC_Geometry][0], m_PF_Params[eHWSC_Geometry].size(), eHWSC_Geometry, m_nMax_PF_Vecs[eHWSC_Geometry]);
  }
#endif
  if (r->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN)
  {
    if (!m_SG_Params[eHWSC_Pixel].empty())
    {
      mfSetParameters(&m_SG_Params[eHWSC_Pixel][0], m_SG_Params[eHWSC_Pixel].size(), eHWSC_Pixel, m_nMax_SG_Vecs[eHWSC_Pixel]);
    }
    if (!m_SG_Params[eHWSC_Vertex].empty())
    {
      mfSetParameters(&m_SG_Params[eHWSC_Vertex][0], m_SG_Params[eHWSC_Vertex].size(), eHWSC_Vertex, m_nMax_SG_Vecs[eHWSC_Vertex]);
    }
#ifndef PS3
    if (!m_SG_Params[eHWSC_Geometry].empty())
    {
      mfSetParameters(&m_SG_Params[eHWSC_Geometry][0], m_SG_Params[eHWSC_Geometry].size(), eHWSC_Geometry, m_nMax_SG_Vecs[eHWSC_Geometry]);
    }
#endif
  }
#else
  if (!m_PF_Params[eHWSC_Pixel].empty())
  {
    mfSetParameters(&m_PF_Params[eHWSC_Pixel][0], m_PF_Params[eHWSC_Pixel].size(), eHWSC_Pixel);
  }
  if (!m_PF_Params[eHWSC_Vertex].empty())
  {
    mfSetParameters(&m_PF_Params[eHWSC_Vertex][0], m_PF_Params[eHWSC_Vertex].size(), eHWSC_Vertex);
  }
  if (!m_PF_Params[eHWSC_Geometry].empty())
  {
    mfSetParameters(&m_PF_Params[eHWSC_Geometry][0], m_PF_Params[eHWSC_Geometry].size(), eHWSC_Geometry);
  }
  if (r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN)
  {
    if (!m_SG_Params[eHWSC_Pixel].empty())
    {
      mfSetParameters(&m_SG_Params[eHWSC_Pixel][0], m_SG_Params[eHWSC_Pixel].size(), eHWSC_Pixel);
    }
    if (!m_SG_Params[eHWSC_Vertex].empty())
    {
      mfSetParameters(&m_SG_Params[eHWSC_Vertex][0], m_SG_Params[eHWSC_Vertex].size(), eHWSC_Vertex);
    }
    if (!m_SG_Params[eHWSC_Geometry].empty())
    {
      mfSetParameters(&m_SG_Params[eHWSC_Geometry][0], m_SG_Params[eHWSC_Geometry].size(), eHWSC_Geometry);
    }
  }
#endif
}

void CHWShader_D3D::mfSetGlobalParams()
{
#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log >= 3)
    gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "--- Set global shader constants...\n");
#endif
#if defined(OPENGL)
  static bool globalMapBuilt = false;//tracks if global has been built
  if(!globalMapBuilt)//create only once
  {
    for(int i=0; i<scSGlobalConstMapCount; ++i)
      sGlobalConsts[i].Init();










    sGlobalConsts[VSCONST_0_025_05_1_OPENGL].shaderType			= SGlobalConstMap::scVSConst;
    sGlobalConsts[VSCONST_0_025_05_1_OPENGL].cpConstName		= "_g_VSConsts0";

    //init instancing stuff
    sGlobalConsts[VSCONST_INSTDATA_OPENGL].shaderType		= SGlobalConstMap::scVSConst;
    sGlobalConsts[VSCONST_INSTDATA_OPENGL].cpConstName	= "_g_InstData";
    //skinning quats and shape deformation stuff





    sGlobalConsts[VSCONST_SKINQUATSL_OPENGL].shaderType		= SGlobalConstMap::scVSConst;
    sGlobalConsts[VSCONST_SKINQUATSL_OPENGL].cpConstName		= "_g_SkinQuatS"; // same as "_g_SkinQuatL"
    sGlobalConsts[VSCONST_SKINQUATSL_OPENGL].isMatrix			= scIs2x4Matrix;
    sGlobalConsts[VSCONST_SHAPEDEFORMATION_OPENGL].shaderType		= SGlobalConstMap::scVSConst;
    sGlobalConsts[VSCONST_SHAPEDEFORMATION_OPENGL].cpConstName	= "_g_ShapeDeformationData";

    globalMapBuilt = true;
  }
#endif

  Vec4 v;
  CD3D9Renderer *r = gcpRendD3D;

#if defined(DIRECT3D10)
  // Preallocate global constant buffer arrays
  if (!m_pCB[eHWSC_Vertex][CB_PER_BATCH])
  {
    int i, j;
    for (i=0; i<CB_MAX; i++)
    {
      for (j=0; j<eHWSC_Max; j++)
      {
#if !defined(PS3)
        int nSize;
        switch (j)
        {
          case eHWSC_Pixel:
            nSize = MAX_CONSTANTS_PS;
            break;
          case eHWSC_Vertex:
            nSize = MAX_CONSTANTS_VS;
            break;
          case eHWSC_Geometry:
            nSize = MAX_CONSTANTS_GS;
            break;
          default:
            assert(0);
            break;
        }


#endif

        m_pCB[j][i] = new ID3D11Buffer* [nSize];
        memset(m_pCB[j][i], 0, sizeof(ID3D11Buffer*)*(nSize));
			}
    }
  }
#endif

#if defined (DIRECT3D10)
  gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_COMMIT_PF | RBPF2_COMMIT_CM;
#else
  //mfSetPF();
  gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_COMMIT_PF;
#endif
}

void CHWShader_D3D::mfSetCameraParams()
{
#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log >= 3)
    gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "--- Set camera shader constants...\n");
#endif
#if defined (DIRECT3D10)
  gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_COMMIT_PF | RBPF2_COMMIT_CM;
#else
  //mfSetCM();
  gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_COMMIT_CM;
#endif
}

bool CHWShader_D3D::mfAddGlobalParameter(SCGParam& Param, EHWShaderClass eSH, bool bSG, bool bCam)
{
  uint32 i;

  if (bCam)
  {
    for (i=0; i<m_CM_Params[eSH].size(); i++)
    {
      SCGParam *pP = &m_CM_Params[eSH][i];
      if (pP->m_Name == Param.m_Name)
        break;
    }
    if (i == m_CM_Params[eSH].size())
    {
      m_CM_Params[eSH].push_back(Param);
#if defined (DIRECT3D10)
      m_nMax_PF_Vecs[eSH] = max(Param.m_dwBind+Param.m_nParameters, m_nMax_PF_Vecs[eSH]);
#endif
      return true;
    }
  }
  else
  if (!bSG)
  {
    for (i=0; i<m_PF_Params[eSH].size(); i++)
    {
      SCGParam *pP = &m_PF_Params[eSH][i];
      if (pP->m_Name == Param.m_Name)
        break;
    }
    if (i == m_PF_Params[eSH].size())
    {
      if (eSH == eHWSC_Pixel)
      {
        if (strnicmp(Param.m_Name.c_str(), "g_PS", 4))
        {
          assert(false);
          iLog->Log("Error: Attempt to use non-PS global parameter in pixel shader");
          return false;
        }
      }
      else
      if (eSH == eHWSC_Vertex)
      {
        if (strnicmp(Param.m_Name.c_str(), "g_VS", 4))
        {
          assert(false);
          iLog->Log("Error: Attempt to use non-VS global parameter in vertex shader");
          return false;
        }
      }
			assert(eSH < eHWSC_Max);
      m_PF_Params[eSH].push_back(Param);
  #if defined (DIRECT3D10)
      m_nMax_PF_Vecs[eSH] = max(Param.m_dwBind+Param.m_nParameters, m_nMax_PF_Vecs[eSH]);
  #endif
      return true;
    }
  }
  else
  {
    for (i=0; i<m_SG_Params[eSH].size(); i++)
    {
      SCGParam *pP = &m_SG_Params[eSH][i];
      if (pP->m_Name == Param.m_Name)
        break;
    }
    if (i == m_SG_Params[eSH].size())
    {
      m_SG_Params[eSH].push_back(Param);
#if defined (DIRECT3D10)
      m_nMax_SG_Vecs[eSH] = max(Param.m_dwBind+Param.m_nParameters, m_nMax_SG_Vecs[eSH]);
#endif
      return true;
    }
  }
  return false;
}

uint32 CHWShader_D3D::mfGetPreprocessFlags(SShaderTechnique *pTech)
{
  uint32 i, j;
  uint32 nFlags = 0;

  for (i=0; i<(uint32)m_Insts.size(); i++)
  {
    SHWSInstance *pInst = &m_Insts[i];
    if (pInst->m_pSamplers.size())
    {
      for (j=0; j<(uint32)pInst->m_pSamplers.size(); j++)
      {
        STexSampler *pSamp = &pInst->m_pSamplers[j];
        if (pSamp && pSamp->m_pTarget)
        {
          SHRenderTarget *pTarg = pSamp->m_pTarget;
          if (pTarg->m_eOrder == eRO_PreProcess)
            nFlags |= pTarg->m_nProcessFlags;
          if (pTech)
          {
            uint32 n = 0;
            for (n=0; n<pTech->m_RTargets.Num(); n++)
            {
              if (pTarg == pTech->m_RTargets[n])
                break;
            }
            if (n == pTech->m_RTargets.Num())
              pTech->m_RTargets.AddElem(pTarg);
          }
        }
      }
    }
  }
  if (pTech)
    pTech->m_RTargets.Shrink();

  return nFlags;
}

