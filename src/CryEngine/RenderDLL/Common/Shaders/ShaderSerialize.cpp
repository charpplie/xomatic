/*=============================================================================
  ShaderSerialize.cpp : implementation of the Shaders serialization manager.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"

#ifdef SHADERS_SERIALIZING

bool CShaderSerialize::_OpenSResource(float fVersion, SSShaderRes *pSR, CShader *pSH, int nCache, CResFile *pRF, bool bReadOnly)
{
  assert(nCache == CACHE_USER || nCache == CACHE_READONLY);

  SSShaderCacheHeader hd;
  ZeroStruct(hd);
  bool bValid = true;
  bool bCheckValid = true;
  if (CRenderer::CV_r_shadersnocompile || CRenderer::CV_r_shadersnosources)
    bCheckValid = false;

  if (!pRF->mfOpen(RA_READ | (CParserBin::m_bEndians ? RA_ENDIANS : 0)))
  {
    pRF->mfClose();
    bValid = false;
  }
  else
  {
    uint64 WriteTimeSrc = 0;
    if (bCheckValid && !pSH->m_ModifTime)
    {
      char nameSrc[256];
      sprintf(nameSrc, "%sCryFX/%s.%s", gRenDev->m_cEF.m_ShadersPath, pSH->GetName(), "cfx");
      FILE *fpSrc = gEnv->pCryPak->FOpen(nameSrc, "rb");
      if (fpSrc)
      {
        WriteTimeSrc = gEnv->pCryPak->GetModificationTime(fpSrc);
        pSH->m_ModifTime = WriteTimeSrc;
        gEnv->pCryPak->FClose(fpSrc);
      }
    }
    uint64 WriteTimeDst = pRF->mfGetModifTime();
    if (bCheckValid && WriteTimeDst != WriteTimeSrc)
    {
      pRF->mfClose();
      bValid = false;
      if (CRenderer::CV_r_shadersdebug==2)
      {
        LogWarning("WARNING: Shader resource '%s' Time mismatch", pRF->mfGetFileName());
      }
    }
    else
    {
      pRF->mfFileSeek(CShaderMan::s_cNameHEAD, 0, SEEK_SET);
      pRF->mfFileRead2(CShaderMan::s_cNameHEAD, sizeof(SSShaderCacheHeader), &hd);
      if (CParserBin::m_bEndians)
        SwapEndian(hd, eBigEndian);
      if (bCheckValid)
      {
        if (hd.m_SizeOf != sizeof(SShaderCacheHeader))
          bValid = false;
        else
        if (fVersion && (hd.m_MajorVer != (int)fVersion || hd.m_MinorVer != (int)(((float)fVersion - (float)(int)fVersion)*10.1f)))
          bValid = false;
        if (!bValid && CRenderer::CV_r_shadersdebug==2)
        {
          LogWarning("WARNING: Shader resource '%s' version mismatch (Resource: %s, Expected: %.1f)", pRF->mfGetFileName(), hd.m_szVer, fVersion);
        }
      }
      if (bValid && nCache == CACHE_USER)
      {
        pRF->mfClose();
        if (!pRF->mfOpen(RA_READ | (CParserBin::m_bEndians ? RA_ENDIANS : 0) | RA_WRITE))
        {
          pRF->mfClose();
          bValid = false;
        }
      }
    }
  }
  if (!bValid && bCheckValid)
  {
    if (nCache == CACHE_USER && !bReadOnly && pSH->m_CRC32)
    {
      if (!pRF->mfOpen(RA_CREATE|(CParserBin::m_bEndians ? RA_ENDIANS : 0)))
        return false;

      SDirEntry de;
      de.Name = CShaderMan::s_cNameHEAD;
      de.eArc = eARC_NONE;
      de.typeID |= FRES_$HEAD;
      de.size = sizeof(SSShaderCacheHeader);
      hd.m_SizeOf = sizeof(SSShaderCacheHeader);
      hd.m_MinorVer = (int)(((float)fVersion - (float)(int)fVersion)*10.1f);
      hd.m_MajorVer = (int)fVersion;
      hd.m_CRC32 = pSH->m_CRC32;
      sprintf(hd.m_szVer, "Ver: %.1f", fVersion);
      SSShaderCacheHeader hdTemp, *pHD;
      pHD = &hd;
      if (CParserBin::m_bEndians)
      {
        hdTemp = hd;
        SwapEndian(hdTemp, eBigEndian);
        pHD = &hdTemp;
      }
      de.pData = pHD;
      pRF->mfFileAdd(&de);

      pRF->mfFlush(pSH->m_ModifTime);
      bValid = true;
    }
    else
    {
      SAFE_DELETE(pRF);
    }
  }
  pSR->m_pRes[nCache] = pRF;
  pSR->m_Header[nCache] = hd;
  pSR->m_bReadOnly[nCache] = bReadOnly;
  if (bValid && !pSH->m_CRC32)
    pSH->m_CRC32 = hd.m_CRC32;

  return bValid;
}

bool CShaderSerialize::OpenSResource(const char *szName,  SSShaderRes *pSR, CShader *pSH, bool bDontUseUserFolder, bool bReadOnly)
{
  CResFile *rfRO = new CResFile(szName);
  float fVersion = (float)FX_CACHE_VER + (float)(FX_SER_CACHE_VER);
  bool bValidRO = _OpenSResource(fVersion, pSR, pSH, bDontUseUserFolder ? CACHE_USER : CACHE_READONLY, rfRO, bDontUseUserFolder ? false : bReadOnly);

  bool bValidUser = false;
#if !defined(SHADER_NO_SOURCES)
  CResFile *rfUser;
  if (!bDontUseUserFolder)
  {
    stack_string szUser = stack_string(gRenDev->m_cEF.m_szUserPath.c_str()) + stack_string(szName);
    rfUser = new CResFile(szUser.c_str());
    bValidUser = _OpenSResource(fVersion, pSR, pSH, CACHE_USER, rfUser, bReadOnly);
  }
#endif

  return (bValidRO || bValidUser);
}

bool CShaderSerialize::CreateSResource(CShader *pSH, SSShaderRes *pSR, CCryName& SName, bool bDontUseUserFolder, bool bReadOnly)
{
  char dstname[512];
  strncpy(dstname, gRenDev->m_cEF.m_ShadersCache, 512);
  strncat(dstname, SName.c_str(), 512);
  strncat(dstname, ".fxb", 512);
  bool bRes = OpenSResource(dstname, pSR, pSH, bDontUseUserFolder, bReadOnly);

  return bRes;
}

SSShaderRes *CShaderSerialize::InitSResource(CShader *pSH, bool bDontUseUserFolder, bool bReadOnly)
{
  SSShaderRes *pSR = NULL;
  CCryName SName = CCryName(pSH->GetName());
  FXSShaderResItor itS = m_SShaderResources.find(SName);
#if SHADER_NO_SOURCES
  bool bCheckValid = false;
#else
  bool bCheckValid = true;
#endif
  if (itS != m_SShaderResources.end())
  {
    pSR = itS->second;
    pSR->m_nRefCount++;
    if (bCheckValid)
    {
      int nCache[2] = {-1,-1};
      if (!bReadOnly || bDontUseUserFolder)
        nCache[0] = CACHE_USER;
      else
      if (!bDontUseUserFolder || bReadOnly)
      {
        nCache[0] = CACHE_USER;
        nCache[1] = CACHE_READONLY;
      }
      for (int i=0; i<2; i++)
      {
        if (nCache[i] < 0 || !pSR->m_pRes[i])
          continue;
        bool bValid = (pSR->m_Header[i].m_CRC32 == pSH->m_CRC32);
        if (!bValid)
        {
          SAFE_DELETE(pSR->m_pRes[i]);
        }
      }
      bool bValid = true;
      if ((!bReadOnly || bDontUseUserFolder) && !pSR->m_pRes[CACHE_USER])
        bValid = false;
      else
      if ((!bDontUseUserFolder || bReadOnly) && !pSR->m_pRes[CACHE_READONLY] && !pSR->m_pRes[CACHE_USER])
        bValid = false;
      if (!bValid)
      {
        CreateSResource(pSH, pSR, SName, bDontUseUserFolder, bReadOnly);
      }
    }
  }
  else
  {
    pSR = new SSShaderRes;
    bool bRes = CreateSResource(pSH, pSR, SName, bDontUseUserFolder, bReadOnly);
    m_SShaderResources.insert(FXSShaderResItor::value_type(SName, pSR));
  }
#if defined(SHADER_NO_SOURCES)
  if (pSR && pSR->m_pRes[CACHE_READONLY])
    pSH->m_CRC32 = pSR->m_Header[CACHE_READONLY].m_CRC32;
#endif

  return pSR;
}

bool CShaderSerialize::ExportHWShader(CHWShader *pShader, SShaderSerializeContext& SC)
{
  if (!pShader)
    return false;

  bool bRes = pShader->Export(SC);

  return bRes;
}
CHWShader *CShaderSerialize::ImportHWShader(SShaderSerializeContext& SC, int nOffs, uint32 CRC32)
{
  CHWShader *pRes = CHWShader::Import(SC, nOffs, CRC32);

  return pRes;
}

bool CShaderSerialize::ExportShader(CShader *pSH)
{
  bool bRes = true;
  //return bRes;

  SSShaderRes *pSR = InitSResource(pSH, ((gRenDev->m_cEF.m_nCombinations > 0) || !CRenderer::CV_r_shadersuserfolder), false);
  int i, j;
  assert(pSR && pSR->m_pRes[CACHE_USER]);
  if (!pSR || !pSR->m_pRes[CACHE_USER])
    return false;

  SShaderSerializeContext SC;

  SC.SSR.m_nParamsOffset = 0;
  SC.SSR.m_nTechOffset = 0;
  SC.SSR.m_nStringsOffset = 0;
  SC.SSR.m_nDataOffset = 0;

  SC.SSR.m_eSHDType = pSH->m_eSHDType;
  SC.SSR.m_eShaderType = pSH->m_eShaderType;
  SC.SSR.m_Flags = pSH->m_Flags;
  SC.SSR.m_Flags2 = pSH->m_Flags2;
  SC.SSR.m_nMDV = pSH->m_nMDV;
  SC.SSR.m_eVertexFormat = pSH->m_eVertexFormat;
  SC.SSR.m_eCull = pSH->m_eCull;
  SC.SSR.m_eShaderType = pSH->m_eShaderType;
  SC.SSR.m_nMaskGenFX = pSH->m_nMaskGenFX;

  SC.SSR.m_nTechniques = pSH->m_HWTechniques.Num();
  SC.SSR.m_nPublicParams = pSH->m_PublicParams.size();
  for (i=0; i<SC.SSR.m_nPublicParams; i++)
  {
    SSShaderParam PR;
    SShaderParam& P = pSH->m_PublicParams[i];
    strncpy(PR.m_Name, P.m_Name, 32);
    PR.m_Type = P.m_Type;
    PR.m_Value = P.m_Value;
    PR.m_nScriptOffs = SC.Strings.Num();
    SC.Strings.AddString(P.m_Script.c_str());
    SC.Params.Add(PR);
  }
  for (i=0; i<SC.SSR.m_nTechniques; i++)
  {
    SSShaderTechnique ST;
    SShaderTechnique *pT = pSH->m_HWTechniques[i];
    ST.m_nPreprocessFlags = pT->GetPreprocessFlags(pSH);
    ST.m_nNameOffs = SC.Strings.Num();
    SC.Strings.AddString(pT->m_Name.c_str());
    ST.m_Flags = pT->m_Flags;
    for (j=0; j<TTYPE_MAX; j++)
    {
      ST.m_nTechnique[j] = pT->m_nTechnique[j];
    }
    ST.m_nREs = pT->m_REs.Num();

    ST.m_nPassesOffs = SC.Passes.Num();
    ST.m_nPasses = pT->m_Passes.Num();
    SC.SSR.m_nPasses += ST.m_nPasses;
    for (j=0; j<ST.m_nPasses; j++)
    {
      SSShaderPass PS;
      SShaderPass &P = pT->m_Passes[j];
      PS.m_RenderState = P.m_RenderState;
      PS.m_eCull = P.m_eCull;
      PS.m_AlphaRef = P.m_AlphaRef;
      PS.m_PassFlags = P.m_PassFlags;

      assert(!(SC.Data.Num() & 0x3));

      PS.m_nVShaderOffs = P.m_VShader ? SC.Data.Num() : -1;
      ExportHWShader(P.m_VShader, SC);
      PS.m_nPShaderOffs = P.m_PShader ? SC.Data.Num() : -1;
      ExportHWShader(P.m_PShader, SC);
      PS.m_nGShaderOffs = P.m_GShader ? SC.Data.Num() : -1;
      ExportHWShader(P.m_GShader, SC);

      SC.Passes.Add(PS);
    }
    for (j=0; j<ST.m_nREs; j++)
    {
      CRendElementBase *pRE = pT->m_REs[j];
      pRE->mfExport(SC);
    }
    SC.Techniques.AddElem(ST);
  }


  TArray<byte> Data;
  uint32 nParamsOffset;
  uint32 nTechOffset;
  uint32 nPassOffset;
  uint32 nStringsOffset;
  uint32 nDataOffset;

  sAddData(Data, SC.SSR);
  sAddData(Data, SC.Params, nParamsOffset);
  sAddData(Data, SC.Techniques, nTechOffset);
  sAddData(Data, SC.Passes, nPassOffset);
  sAddData(Data, SC.Strings, nStringsOffset);
  sAddData(Data, SC.Data, nDataOffset);

  SSShader *pSSR = (SSShader *)&Data[0];
  pSSR->m_nParamsOffset = nParamsOffset;
  pSSR->m_nTechOffset = nTechOffset;
  pSSR->m_nPassOffset = nPassOffset;
  pSSR->m_nStringsOffset = nStringsOffset;
  pSSR->m_nDataOffset = nDataOffset;
  pSSR->m_nDataSize = SC.Data.Num();
  pSSR->m_nStringsSize = SC.Strings.Num();

  int nLen = Data.Num();
  SDirEntry de;
  char sName[128];
#if defined(__GNUC__)
  sprintf(sName, "(%llx)", pSH->m_nMaskGenFX);
#else
  sprintf(sName, "(%I64x)", pSH->m_nMaskGenFX);
#endif
  de.Name = CCryNameCRC(sName);
  de.eArc = eARC_NONE;
  de.size = nLen;
  de.pData = &Data[0];
  de.flags = 0;
  pSR->m_pRes[CACHE_USER]->mfFileAdd(&de);
  pSR->m_pRes[CACHE_USER]->mfFlush(pSH->m_ModifTime);

  return bRes;
}

float g_fTime0;
float g_fTime1;
float g_fTime2;

bool CShaderSerialize::ImportShader(CShader *pSH)
{
  //return false;
  bool bRes = true;

  float fTime0 = iTimer->GetAsyncCurTime();
  SSShaderRes *pSR = InitSResource(pSH, ((gRenDev->m_cEF.m_nCombinations > 0 && !gRenDev->m_cEF.m_bActivatePhase) || !CRenderer::CV_r_shadersuserfolder), true);
  if (!pSR || (!pSR->m_Header[CACHE_USER].m_CRC32 && !pSR->m_Header[CACHE_READONLY].m_CRC32))
    return false;

  CShader *pSave = gRenDev->m_RP.m_pShader;
  gRenDev->m_RP.m_pShader = pSH;
	assert(gRenDev->m_RP.m_pShader != 0);

  int i, j;
  char sName[128];
#if defined(__GNUC__)
  sprintf(sName, "(%llx)", pSH->m_nMaskGenFX);
#else
  sprintf(sName, "(%I64x)", pSH->m_nMaskGenFX);
#endif
  CCryNameCRC CName = CCryNameCRC(sName);
  SDirEntry *pDE = NULL;
  CResFile *pRes = NULL;
  for (i=0; i<2; i++)
  {
    pRes = pSR->m_pRes[i];
    if (!pRes)
      continue;
    pDE = pRes->mfGetEntry(CName);
    if (pDE)
      break;
  }
  if (!pDE || !pRes)
  {
    gRenDev->m_RP.m_pShader = pSave;
    return false;
  }
  int nSize = pRes->mfFileRead(pDE);
  byte *pData = (byte *)pRes->mfFileGetBuf(pDE);
  if (!pData)
  {
    gRenDev->m_RP.m_pShader = pSave;
    return false;
  }
  byte *pSrc = pData;
  g_fTime0 += iTimer->GetAsyncCurTime() - fTime0;

  float fTime1 = iTimer->GetAsyncCurTime();

  SShaderSerializeContext SC;
  memcpy(&SC.SSR, pSrc, sizeof(SC.SSR));
  if (SC.SSR.m_nPublicParams)
  {
    SC.Params.ReserveNoClear(SC.SSR.m_nPublicParams);
    memcpy(&SC.Params[0], &pSrc[SC.SSR.m_nParamsOffset], sizeof(SSShaderParam)*SC.SSR.m_nPublicParams);
  }
  if (SC.SSR.m_nTechniques)
  {
    SC.Techniques.ReserveNoClear(SC.SSR.m_nTechniques);
    memcpy(&SC.Techniques[0], &pSrc[SC.SSR.m_nTechOffset], sizeof(SSShaderTechnique)*SC.SSR.m_nTechniques);
  }
  if (SC.SSR.m_nPasses)
  {
    SC.Passes.ReserveNoClear(SC.SSR.m_nPasses);
    memcpy(&SC.Passes[0], &pSrc[SC.SSR.m_nPassOffset], sizeof(SSShaderPass)*SC.SSR.m_nPasses);
  }
  if (SC.SSR.m_nStringsSize)
  {
    SC.Strings.ReserveNoClear(SC.SSR.m_nStringsSize);
    memcpy(&SC.Strings[0], &pSrc[SC.SSR.m_nStringsOffset], SC.SSR.m_nStringsSize);
  }
  if (SC.SSR.m_nDataSize)
  {
    SC.Data.ReserveNoClear(SC.SSR.m_nDataSize);
    memcpy(&SC.Data[0], &pSrc[SC.SSR.m_nDataOffset], SC.SSR.m_nDataSize);
  }

  pRes->mfFileClose(pDE);

  g_fTime1 += iTimer->GetAsyncCurTime() - fTime1;

  float fTime2 = iTimer->GetAsyncCurTime();

  pSH->m_eSHDType = SC.SSR.m_eSHDType;
  pSH->m_eShaderType = SC.SSR.m_eShaderType;
  pSH->m_Flags = SC.SSR.m_Flags;
  pSH->m_Flags2 = SC.SSR.m_Flags2;
  pSH->m_nMDV = SC.SSR.m_nMDV;
  pSH->m_eVertexFormat = SC.SSR.m_eVertexFormat;
  pSH->m_eCull = SC.SSR.m_eCull;
  pSH->m_eShaderType = SC.SSR.m_eShaderType;
  pSH->m_nMaskGenFX = SC.SSR.m_nMaskGenFX;

  for (i=0; i<SC.SSR.m_nPublicParams; i++)
  {
    SSShaderParam& PR = SC.Params[i];
    SShaderParam P;
    strncpy(P.m_Name, PR.m_Name, 32);
    P.m_Type = PR.m_Type;
    P.m_Value = PR.m_Value;
    P.m_Script = sString(PR.m_nScriptOffs, SC.Strings);
    pSH->m_PublicParams.push_back(P);
  }
  for (i=0; i<SC.SSR.m_nTechniques; i++)
  {
    SSShaderTechnique& ST = SC.Techniques[i];
    SShaderTechnique *pT = new SShaderTechnique;
    pT->m_Name = sString(ST.m_nNameOffs, SC.Strings);
    pT->m_Flags = ST.m_Flags;
    pT->m_nPreprocessFlags = ST.m_nPreprocessFlags;
    for (j=0; j<TTYPE_MAX; j++)
    {
      pT->m_nTechnique[j] = ST.m_nTechnique[j];
    }
    //ST.m_nREs = pT->m_REs.Num();
    //SC.Techniques.AddElem(ST);

    if (ST.m_nPasses)
    {
      int nOffs = ST.m_nPassesOffs;
      for (j=0; j<ST.m_nPasses; j++)
      {
        SSShaderPass& PS = SC.Passes[j+nOffs];
        SShaderPass *P = pT->m_Passes.AddIndex(1);
        P->m_RenderState = PS.m_RenderState;
        P->m_eCull = PS.m_eCull;
        P->m_AlphaRef = PS.m_AlphaRef;
        P->m_PassFlags = PS.m_PassFlags;

        P->m_VShader = ImportHWShader(SC, PS.m_nVShaderOffs, pSH->m_CRC32);
        P->m_PShader = ImportHWShader(SC, PS.m_nPShaderOffs, pSH->m_CRC32);
        P->m_GShader = ImportHWShader(SC, PS.m_nGShaderOffs, pSH->m_CRC32);
      }
    }
    for (j=0; j<ST.m_nREs; j++)
    {
      //SSHRenderTarget RT;
      //CRendElementBase *pRE = pT->m_REs[j];
      //pRE->mfExport(SC);
    }

    pSH->m_HWTechniques.AddElem(pT);
  }
  gRenDev->m_RP.m_pShader = pSave;

  g_fTime2 += iTimer->GetAsyncCurTime() - fTime2;

  return bRes;
}

#endif
