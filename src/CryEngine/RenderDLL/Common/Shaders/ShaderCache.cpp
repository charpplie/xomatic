/*=============================================================================
  ShaderCache.cpp : implementation of the Shaders cache management.
  Copyright (c) 2001-2009 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "I3DEngine.h"

#if !defined (XENON) && !defined(PS3)

static TArray<SPreprocessNode *> sfxNodeStack;

bool CParserBin::_PreprocessTree_Expr(const uint32 *pTokens, uint32& nT, int nTSize, SPreprocessTree *pTree, SPreprocessNode *pNode)
{
  int nLevel = 0;
  bool bWas = false;
  bool bWord = false;
  for (; nT<nTSize; nT++)
  {
    uint32 nToken = pTokens[nT];
    if (bWas)
    {
      assert(nToken == eT_or || nToken == eT_and);
      if (nToken == eT_or || nToken == eT_and)
      {
        bWas = false;
        continue;
      }
    }
    pNode->m_Expression.push_back(nToken);
    switch (nToken)
    {
      case eT_br_rnd_1:  // '('
        nLevel++;
        break;
      case eT_br_rnd_2:  // ')'
        nLevel--;
        if (nLevel < 0)
          return false;
        bWord = true;
        break;
      case eT_or:       // '|'
      case eT_and:      // '&'
        bWas = true;
        bWord = false;
        break;
      case eT_excl:
        bWord = false;
        break;
      default:
        {
          if (!nLevel && bWord)
          {
            pNode->m_Expression.erase(pNode->m_Expression.begin()+pNode->m_Expression.size()-1);
            return true;
          }
          bWord = true;
          int bLocal = -1;
          MapPreprocessFlagsItor pFlag = pTree->m_MapFlags.find(nToken);
          if (pFlag != pTree->m_MapFlags.end())
            bLocal = 0;
          else
          {
            pFlag = pTree->m_MapFlagsLocal.find(nToken);
            if (pFlag != pTree->m_MapFlagsLocal.end())
              bLocal = 1;
          }
          if (bLocal >= 0)
          {
            uint32 nFlagID = pFlag->second & 0xffff;
            uint32 nFlagType = pFlag->second >> 16;
            SPreprocessFlagDesc *pDesc = NULL;
            switch (nFlagType)
            {
              case 0:           // RT_
                pDesc = &pTree->m_RTDescs[nFlagID];
                pNode->m_RTMask |= pDesc->m_nFlag;
                break;
              case 1:           // GL_
                if (bLocal)
                  pDesc = &pTree->m_GLDescsLocal[nFlagID];
                else
                  pDesc = &pTree->m_GLDescs[nFlagID];
                pNode->m_GLMask |= pDesc->m_nFlag;
                break;
              case 2:           // LT_
                pDesc = &pTree->m_LTDescs[nFlagID];
                pNode->m_LTMask |= pDesc->m_nFlag;
                break;
              case 3:           // MD_
                pDesc = &pTree->m_MDDescs[nFlagID];
                pNode->m_MDMask |= pDesc->m_nFlag;
                break;
              case 4:           // MDV_
                pDesc = &pTree->m_MDVDescs[nFlagID];
                pNode->m_MDVMask |= pDesc->m_nFlag;
                break;
              default:
                assert(0);
                break;
            }
          }
          else
          {
            const char *szStr = CParserBin::GetString(nToken);
            int nnn = 0;
          }
        }
        break;
    }
  }
  return true;
}

bool CParserBin::IsPreprocessExprTrue(SPreprocessTree *pTree, const std::vector<uint32>& Expression, SPreprocessMasks& Masks)
{
  byte bRes[64];
  byte bOr[64];
  int nLevel = 0;
  int i;
  int nT = 0;
  while (true)
  {
    uint32 nToken = Expression[nT];
    if (nToken == eT_br_rnd_1) // check for '('
    {
      nT++;
      int n = 0;
      std::vector<uint32> tmpBuf;
      while (true)
      {
        int nTok = Expression[nT];
        if (nTok == eT_br_rnd_1) // check for '('
          n++;
        else
        if (nTok == eT_br_rnd_2) // check for ')'
        {
          if (!n)
          {
            tmpBuf.push_back(0);
            nT++;
            break;
          }
          n--;
        }
        else
        if (nTok == 0)
          return false;
        tmpBuf.push_back(nTok);
        nT++;
      }
      bRes[nLevel] = IsPreprocessExprTrue(pTree, tmpBuf, Masks);
      nLevel++;
      bOr[nLevel] = 255;
    }
    else
    {
      uint32 nTok = Expression[nT++];
      bool bNeg = false;
      if (nTok == eT_excl)
      {
        bNeg = true;
        nTok = Expression[nT++];
      }
      MapPreprocessFlagsItor pFlag = pTree->m_MapFlags.find(nTok);
      if (pFlag != pTree->m_MapFlags.end())
      {
        uint32 nFlagID = pFlag->second & 0xffff;
        uint32 nFlagType = pFlag->second >> 16;
        SPreprocessFlagDesc *pDesc = NULL;
        switch (nFlagType)
        {
        case 0:           // RT_
          pDesc = &pTree->m_RTDescs[nFlagID];
          bRes[nLevel] = (Masks.nRTSet & pDesc->m_nFlag) != 0;
          break;
        case 1:           // GL_
          pDesc = &pTree->m_GLDescs[nFlagID];
          bRes[nLevel] = (Masks.nGLSet & pDesc->m_nFlag) != 0;
          break;
        case 2:           // LT_
          pDesc = &pTree->m_LTDescs[nFlagID];
          bRes[nLevel] = (Masks.nLTSet & pDesc->m_nFlag) != 0;
          break;
        }
      }
      else
      {
        const SMacroBinFX *pFound = FindMacro(nTok, m_Macros[0]);
        if (!pFound)
          pFound = FindMacro(nTok, m_StaticMacros);
        if (!pFound)
          pFound = FindMacro(nTok, m_Macros[1]);
        bRes[nLevel] = (pFound) ? true : false;
      }
      if (bNeg)
        bRes[nLevel] = !bRes[nLevel];
      nLevel++;
      bOr[nLevel] = 255;
    }
    uint32 nTok = Expression[nT];
    if (nTok == eT_or)
    {
      bOr[nLevel] = true;
      nT++;
    }
    else
    if (nTok == eT_and)
    {
      bOr[nLevel] = false;
      nT++;
    }
    else
      break;
  }
  byte Res = false;
  for (i=0; i<nLevel; i++)
  {
    if (!i)
      Res = bRes[i];
    else
    {
      assert(bOr[i] != 255);
      if (bOr[i])
        Res = Res | bRes[i];
      else
        Res = Res & bRes[i];
    }
  }
  return Res != 0;
}

SPreprocessNode *CParserBin::_PreprocessTree_if(const uint32 *pTokens, uint32& nT, int nTSize, SPreprocessTree *pTree, SPreprocessNode *pNode, bool bRecurse)
{
  SPreprocessNode *pNewNode = new SPreprocessNode;
  if (!sfxNodeStack.size())
  {
    assert(!pNode);
    pTree->m_Root.push_back(pNewNode);
  }
  else
  {
    assert (pNode);
    if (pNode)
    {
      pNode->m_Nodes[pNode->m_nNode & 0xff].push_back(pNewNode);
    }
  }
  bool bRes = _PreprocessTree_Expr(pTokens, nT, nTSize, pTree, pNewNode);
  if (bRecurse)
    sfxNodeStack.push_back(pNode);

  return pNewNode;
}

bool CParserBin::_PreprocessBuildTree(DynArray<uint32>& Tokens, SPreprocessTree *pTree, SPreprocessNode *pNode)
{
  bool bRet = true;
  uint32 nTokenParam;
  int nPass = 0;

  int nNode = -1;
  uint32 nT = 0;
  const uint32 *pTokens = &Tokens[0];
  const uint32 nTSize = Tokens.size();
  while (nT < nTSize)
  {
    uint32 nToken = pTokens[nT++];
    if (!nToken)
      break;
    bool bFirst = false;
    switch (nToken)
    {
    case eT_skip:
      break;
    case eT_skip_1:
      /*nToken = pTokens[nT++];
      if (nToken >= eT_if && nToken <= eT_elif)
        _PreprocessTree_if(pTokens, nT, nTSize, pTree, pNode, false);
        */
      break;
    case eT_skip_2:
      break;
    case eT_include:
      {
        nTokenParam = pTokens[nT++];
        const char *szName = GetString(nTokenParam, false);
        assert(szName);
        SShaderBin *pBin = gRenDev->m_cEF.m_Bin.GetBinShader(szName, true, 0);
        if (!pBin)
        {
          iLog->Log("Warning: Couldn't find include file '%s'", szName);
          assert(0);
        }
        else
        {
          assert(pBin);
          MergeTable(pBin);
          pBin->Lock();
          _PreprocessBuildTree(pBin->m_Tokens, pTree, pNode);
          pBin->Unlock();
        }
      }
      break;
    case eT_define:
    case eT_define_2:
      {
        uint32 nMacro = 0;
        int n = nPass;
        uint32 nMask = 0;
        nTokenParam = pTokens[nT++];
        /*const char *szname = GetString(nTokenParam);
        if (!stricmp(szname, "%_SHADOW_GEN"))
        {
        int nnn = 0;
        }*/
        const uint32 *pMacro = &pTokens[nT];
        while (pMacro[nMacro])
          nMacro++;
        if (nToken == eT_define_2)
        {
          n = 1;
          if (nMacro)
            nMask = GetInt(pMacro[0]);
        }
        AddMacro(nTokenParam, pMacro, nMacro, nMask, m_Macros[n]);
        nT += nMacro+1;
      }
      break;
    case eT_undefine:
      {
        uint32 nMacro = 0;
        nTokenParam = pTokens[nT++];
        int n = nPass;
        FXMacroBinItor it = m_Macros[nPass].find(nTokenParam);
        if (it == m_Macros[n].end() && !nPass)
        {
          it = m_Macros[1].find(nTokenParam);
          n = 1;
        }
        if (it == m_Macros[n].end())
        {
          Warning("Couldn't find macro '%s'", GetString(nTokenParam));
        }
        else
        {
          m_Macros[n].erase(nTokenParam);
        }
      }
      break;
    case eT_if:
    case eT_ifdef:
    case eT_ifndef:
    case eT_if_2:
    case eT_ifdef_2:
    case eT_ifndef_2:
      pNode = _PreprocessTree_if(pTokens, nT, nTSize, pTree, pNode, true);
      break;
    case eT_elif:
      {
        int nLevel = sfxNodeStack.Num()-1;
        if (nLevel < 0)
        {
          assert(0);
          Warning("#elif without #ifdef");
          return false;
        }
        pNode->m_nNode = 1 | 0x8000;
        pNode = _PreprocessTree_if(pTokens, nT, nTSize, pTree, pNode, true);
        break;
      }
      break;
    case eT_else:
      {
        if (!pNode || pNode->m_nNode)
        {
          assert(0);
          Warning("#else without #ifdef");
          return false;
        }
        pNode->m_nNode = 1;
      }
      break;
    case eT_endif:
      {
        if (!pNode || !sfxNodeStack.size())
        {
          assert(0);
          Warning( "#endif without #ifdef");
          return false;
        }
        while (true)
        {
          int nLevel = sfxNodeStack.size()-1;
          if (nLevel < 0)
          {
            assert(0);
            pNode = NULL;
            break;
          }
          pNode = sfxNodeStack[nLevel];
          sfxNodeStack.Remove(nLevel);
          if (!pNode || !(pNode->m_nNode & 0x8000))
            break;
          pNode->m_nNode &= ~0x8000;
        }
      }
      break;
    case eT_warning:
      {
        const char *szStr = GetString(pTokens[nT++]);
        Warning(szStr);
      }
      break;
    case eT_register_env:
      {
        const char *szStr = GetString(pTokens[nT++]);
        if (szStr && !strcmp(szStr, "LIGHT_SETUP"))
          m_bNewLightSetup = true;
        fxRegisterEnv(szStr);
      }
      break;
    default:
      {
        if (pNode)
          pNode->m_nCode[pNode->m_nNode] = 1;
      }
      break;
    }
  }
  return bRet;
}

bool CParserBin::PreprocessBuildTree(DynArray<uint32>& Tokens, FXShaderToken *pSrcTable, SPreprocessTree *pTree)
{
  bool bRes = true;

  sfxNodeStack.SetUse(0);
  m_Macros[0].clear();
  m_Table = *pSrcTable;

  bRes = _PreprocessBuildTree(Tokens, pTree, NULL);

  assert(!sfxIFDef.Num());
  assert(!sfxIFIgnore.Num());

  return bRes;
}

MapPreprocessFlags SPreprocessTree::m_MapFlags;
std::vector<SPreprocessFlagDesc> SPreprocessTree::m_GLDescs;
std::vector<SPreprocessFlagDesc> SPreprocessTree::m_RTDescs;
std::vector<SPreprocessFlagDesc> SPreprocessTree::m_LTDescs;

SPreprocessTree *CParserBin::BuildPreprocessTree(DynArray<uint32>* pSHData, FXShaderToken* Table, CShader *ef)
{
  uint32 i, j;
  bool bRes = true;

  /*{
    TArray<char> TempTx;
    CParserBin::ConvertToAscii(&(*pSHData)[0], pSHData->size(), *Table, TempTx, true);
    FILE *fp = gEnv->pCryPak->FOpen("tree.txt", "w");
    if (fp)
    {
      gEnv->pCryPak->FPrintf(fp, "%s", &TempTx[0]);
      gEnv->pCryPak->FClose (fp);
    }
  }*/

  SPreprocessTree *pTree = new SPreprocessTree;
  // Fill global flags info
  if (SPreprocessTree::m_MapFlags.size() == 0)
  {
    CShaderMan::MapNameFlagsItor it;
    for (it=gRenDev->m_cEF.m_pShaderCommonGlobalFlag.begin(); it!=gRenDev->m_cEF.m_pShaderCommonGlobalFlag.end(); ++it)
    {
      uint64 nMask = it->second;
      if (nMask)
      {
        for (j=0; j<64; j++)
        {
          if ((PLATFORM_I64(1)<<j) == nMask)
            break;
        }
        assert(j < 64);
        if (j < 64)
        {
          if (SPreprocessTree::m_GLDescs.size() <= j)
            SPreprocessTree::m_GLDescs.resize(j+1);
          SPreprocessFlagDesc DS;
          DS.m_Name = it->first.c_str();
          DS.m_nFlag = nMask;
          SPreprocessTree::m_GLDescs[j] = DS;

          bool bKey = false;
          uint32 nName = CParserBin::fxToken(DS.m_Name.c_str(), &bKey);
          if (!bKey)
            nName = CParserBin::GetCRC32(DS.m_Name.c_str());
          MapPreprocessFlagsItor pCheck = SPreprocessTree::m_MapFlags.find(nName);
          if (pCheck == SPreprocessTree::m_MapFlags.end())
            SPreprocessTree::m_MapFlags.insert(MapPreprocessFlagsItor::value_type(nName, 0x10000 | j));
        }
      }
    }
  }
  // Fill runtime flags info
  SShaderGen *pGen = gRenDev->m_cEF.m_pGlobalExt;
  for (i=0; i<pGen->m_BitMask.Num(); i++)
  {
    SShaderGenBit *pBit = pGen->m_BitMask[i];
    if (pBit->m_Mask)
    {
      for (j=0; j<64; j++)
      {
        if ((PLATFORM_I64(1)<<j) == pBit->m_Mask)
          break;
      }
      assert(j < 64);
      if (j < 64)
      {
        if (SPreprocessTree::m_RTDescs.size() <= j)
          SPreprocessTree::m_RTDescs.resize(j+1);
        SPreprocessFlagDesc DS;
        DS.m_Name = pBit->m_ParamName.c_str();
        DS.m_nFlag = pBit->m_Mask;
        SPreprocessTree::m_RTDescs[j] = DS;

        bool bKey = false;
        uint32 nName = CParserBin::fxToken(DS.m_Name.c_str(), &bKey);
        if (!bKey)
          nName = CParserBin::GetCRC32(DS.m_Name.c_str());
        MapPreprocessFlagsItor pCheck = SPreprocessTree::m_MapFlags.find(nName);
        if (pCheck == SPreprocessTree::m_MapFlags.end())
          SPreprocessTree::m_MapFlags.insert(MapPreprocessFlagsItor::value_type(nName, j));
      }
    }

    // Fill light flags info
    {
      if (SPreprocessTree::m_LTDescs.size() <= 1)
        SPreprocessTree::m_LTDescs.resize(2);
      SPreprocessFlagDesc DS;
      DS.m_Name = "%_LT_LIGHTS";
      DS.m_nFlag = -1;
      SPreprocessTree::m_LTDescs[0] = DS;
      bool bKey = false;
      uint32 nName = CParserBin::fxToken(DS.m_Name.c_str(), &bKey);
      MapPreprocessFlagsItor pCheck = pTree->m_MapFlags.find(nName);
      if (pCheck == pTree->m_MapFlags.end())
        pTree->m_MapFlags.insert(MapPreprocessFlagsItor::value_type(nName, 0x20000 | 0));

      DS.m_Name = "%_LT_NUM";
      DS.m_nFlag = -1;
      SPreprocessTree::m_LTDescs[1] = DS;
      nName = CParserBin::fxToken(DS.m_Name.c_str(), &bKey);
      pCheck = SPreprocessTree::m_MapFlags.find(nName);
      if (pCheck == SPreprocessTree::m_MapFlags.end())
        SPreprocessTree::m_MapFlags.insert(MapPreprocessFlagsItor::value_type(nName, 0x20000 | 1));
    }
  }
  if (ef->m_pGenShader && ef->m_pGenShader->m_ShaderGenParams)
  {
    SShaderGen *pGen = ef->m_pGenShader->m_ShaderGenParams;

    for (i=0; i<pGen->m_BitMask.Num(); i++)
    {
      SShaderGenBit *pBit = pGen->m_BitMask[i];
      for (j=0; j<SPreprocessTree::m_GLDescs.size(); j++)
      {
        if (SPreprocessTree::m_GLDescs[j].m_Name == CCryName(pBit->m_ParamName.c_str()))
          break;
      }
      if (j != SPreprocessTree::m_GLDescs.size())
        continue;
      if (pBit->m_Mask)
      {
        for (j=0; j<64; j++)
        {
          if ((PLATFORM_I64(1)<<j) == pBit->m_Mask)
            break;
        }
        assert(j < 64);
        if (j < 64)
        {
          if (pTree->m_GLDescsLocal.size() <= j)
            pTree->m_GLDescsLocal.resize(j+1);
          SPreprocessFlagDesc DS;
          DS.m_Name = pBit->m_ParamName.c_str();
          DS.m_nFlag = pBit->m_Mask;
          pTree->m_GLDescsLocal[j] = DS;

          bool bKey = false;
          uint32 nName = CParserBin::fxToken(DS.m_Name.c_str(), &bKey);
          if (!bKey)
            nName = CParserBin::GetCRC32(DS.m_Name.c_str());
          MapPreprocessFlagsItor pCheck = pTree->m_MapFlagsLocal.find(nName);
          if (pCheck == pTree->m_MapFlagsLocal.end())
            pTree->m_MapFlagsLocal.insert(MapPreprocessFlagsItor::value_type(nName, 0x10000 | j));
        }
      }
    }
  }

  bRes = PreprocessBuildTree(*pSHData, Table, pTree);

  if (!bRes)
  {
    SAFE_DELETE (pTree);
  }

  return pTree;
}
#endif

bool CShader::mfPrecache(SShaderCombination& cmb)
{
  int nAsync = CRenderer::CV_r_shadersasynccompiling;
  CRenderer::CV_r_shadersasynccompiling = 0;

  uint32 i, j;
  bool bRes = true;
  gRenDev->m_RP.m_pShader = this;
  for (i=0; i<m_HWTechniques.Num(); i++)
  {
    SShaderTechnique *pTech = m_HWTechniques[i];
    for (j=0; j<pTech->m_Passes.Num(); j++)
    {
      SShaderPass& Pass = pTech->m_Passes[j];
      SShaderCombination c = cmb;
      gRenDev->m_RP.m_FlagsShader_MD = cmb.m_MDMask;
      if (Pass.m_PShader)
        bRes &= Pass.m_PShader->mfPrecache(cmb, false);
      cmb.m_MDMask = gRenDev->m_RP.m_FlagsShader_MD;
      if (Pass.m_VShader)
        bRes &= Pass.m_VShader->mfPrecache(cmb, false);
      cmb = c;
    }
  }
  CRenderer::CV_r_shadersasynccompiling = nAsync;

  return bRes;
}

SShaderGenComb *CShaderMan::mfGetShaderGenInfo(const char *nmFX)
{
  SShaderGenComb *c = NULL;
  uint32 i;
  char nameExt[128];
  for (i=0; i<m_SGC.size(); i++)
  {
    c = &m_SGC[i];
    if (!stricmp(c->Name.c_str(), nmFX))
      break;
  }
  SShaderGenComb cmb;
  if (i == m_SGC.size())
  {
    c = NULL;
    sprintf(nameExt, "Shaders/%s.ext", nmFX);
    cmb.pGen = mfCreateShaderGenInfo(nameExt);
    cmb.Name = CCryName(nmFX);
    m_SGC.push_back(cmb);
    c = &m_SGC[i];
  }
  return c;
}

static uint64 sGetGL(char **s, CCryName& name, uint32& nHWFlags)
{
  uint32 i;

  nHWFlags = 0;
  SShaderGenComb *c = NULL;
  const char *m = strchr(name.c_str(), '@');
  if (!m)
    m = strchr(name.c_str(), '/');
  assert(m);
  char nmFX[128], nameExt[128];
  if (m)
  {
    strncpy(nmFX, name.c_str(), m-name.c_str());
    nmFX[m-name.c_str()] = 0;
  }
  else
    return -1;
  c = gRenDev->m_cEF.mfGetShaderGenInfo(nmFX);
  if (!c || !c->pGen || !c->pGen->m_BitMask.Num())
    return 0;
  uint64 nGL = 0;
  SShaderGen *pG = c->pGen;
  for (i=0; i<pG->m_BitMask.Num(); i++)
  {
    SShaderGenBit *pBit = pG->m_BitMask[i];
    if (pBit->m_nDependencySet & (SHGD_HW_BILINEARFP16 | SHGD_HW_SEPARATEFP16))
      nHWFlags |= pBit->m_nDependencySet;
  }
  while (true)
  {
    char theChar;
    int n = 0;
    while ((theChar = **s) != 0)
    {
      if (theChar == ')' || theChar == '|')
      {
        nameExt[n] = 0;
        break;
      }
      nameExt[n++] = theChar;
      ++*s;
    }
    if (!nameExt[0])
      break;
    for (i=0; i<pG->m_BitMask.Num(); i++)
    {
      SShaderGenBit *pBit = pG->m_BitMask[i];
      if (!stricmp(pBit->m_ParamName.c_str(), nameExt))
      {
        nGL |= pBit->m_Mask;
        break;
      }
    }
    if (i == pG->m_BitMask.Num())
    {
      if (!strncmp(nameExt, "0x", 2))
      {
        //nGL |= shGetHex(&nameExt[2]);
      }
      else
      {
        //assert(0);
        if (CRenderer::CV_r_shadersdebug)
          iLog->Log("WARNING: Couldn't find global flag '%s' in shader '%s' (skipped)", nameExt, c->Name.c_str());
      }
    }
    if (**s == '|')
      ++*s;
  }
  return nGL;
}

static uint64 sGetRT(char **s)
{
  uint32 i;

  SShaderGen *pG = gRenDev->m_cEF.m_pGlobalExt;
  if (!pG)
    return 0;
  uint64 nRT = 0;
  char nm[128];
  while (true)
  {
    char theChar;
    int n = 0;
    while ((theChar = **s) != 0)
    {
      if (theChar == ')' || theChar == '|')
      {
        nm[n] = 0;
        break;
      }
      nm[n++] = theChar;
      ++*s;
    }
    if (!nm[0])
      break;
    for (i=0; i<pG->m_BitMask.Num(); i++)
    {
      SShaderGenBit *pBit = pG->m_BitMask[i];
      if (!stricmp(pBit->m_ParamName.c_str(), nm))
      {
        nRT |= pBit->m_Mask;
        break;
      }
    }
    if (i == pG->m_BitMask.Num())
    {
      //assert(0);
//      iLog->Log("WARNING: Couldn't find runtime flag '%s' (skipped)", nm);
    }
    if (**s == '|')
      ++*s;
  }
  return nRT;
}

static int sEOF(bool bFromFile, char *pPtr, FILE *fp)
{
  int nStatus;
  if (bFromFile)
    nStatus = gEnv->pCryPak->FEof(fp);
  else
  {
    SkipCharacters(&pPtr, kWhiteSpace);
    if (!*pPtr)
      nStatus = 1;
    else
      nStatus = 0;
  }
  return nStatus;
}

void CShaderMan::mfCloseShadersCache(int nID)
{
  if (m_FPCacheCombinations[nID])
  {
    gEnv->pCryPak->FClose(m_FPCacheCombinations[nID]);
    m_FPCacheCombinations[nID] = NULL;
  }
}

void sSkipLine(char *& s)
{
  char *sEnd = strchr(s, '\n');
  if (sEnd)
  {
    sEnd++;
    s = sEnd;
  }
}

static void sIterateHW_r(FXShaderCacheCombinations *Combinations, SCacheCombination& cmb, int i, uint64 nHW, const char *szName)
{
  string str;
  gRenDev->m_cEF.mfInsertNewCombination(cmb.nGL, cmb.nRT, cmb.nLT, cmb.nMD, cmb.nMDV, cmb.eCL, szName, 0, &str, false);
  CCryName nm = CCryName(str.c_str());
  FXShaderCacheCombinationsItor it = Combinations->find(nm);
  if (it == Combinations->end())
  {
    cmb.CacheName = nm;
    Combinations->insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
  }
  for (int j=i; j<64; j++)
  {
    if (((uint64)1<<j) & nHW)
    {
      cmb.nGL &= ~((uint64)1<<j);
      sIterateHW_r(Combinations, cmb, j+1, nHW, szName);
      cmb.nGL |= ((uint64)1<<j);
      sIterateHW_r(Combinations, cmb, j+1, nHW, szName);
    }
  }

}

void CShaderMan::mfGetShaderListPath(stack_string& nameOut, int nType)
{
  if (nType == 0)
	  nameOut = CRenderer::CV_r_shadersuserfolder ? stack_string(m_szUserPath.c_str()) + stack_string("shaders/shaderlist.txt") : stack_string("shaders/cache/shaderlist.txt");
  else
    nameOut = CRenderer::CV_r_shadersuserfolder ? stack_string(m_szUserPath.c_str()) + stack_string("shaders/cache/shaderlistactivate.txt") : stack_string("shaders/cache/shaderlistactivate.txt");
}

void CShaderMan::mfMergeShadersCombinations(FXShaderCacheCombinations *Combinations, int nType)
{
  FXShaderCacheCombinationsItor itor;
  for (itor=m_ShaderCacheCombinations[nType].begin(); itor!=m_ShaderCacheCombinations[nType].end(); itor++)
  {
    SCacheCombination *cmb = &itor->second;
    FXShaderCacheCombinationsItor it = Combinations->find(cmb->CacheName);
    if (it == Combinations->end())
    {
      Combinations->insert(FXShaderCacheCombinationsItor::value_type(cmb->CacheName, *cmb));
    }
  }
}


void CShaderMan::mfInitShadersCache(bool bForLevel, FXShaderCacheCombinations *Combinations, const char* pCombinations, int nType)
{
  char str[2048];
  bool bFromFile = (Combinations == NULL);
  stack_string nameComb;

  FILE *fp = NULL;
  if (bFromFile)
  {
    mfGetShaderListPath(nameComb, nType);
    fp = gEnv->pCryPak->FOpen(nameComb.c_str(), "r+");
    if (!fp)
      fp = gEnv->pCryPak->FOpen(nameComb.c_str(), "w+");
    if (!fp)
    {
      gEnv->pCryPak->AdjustFileName(nameComb.c_str(), str, 0);
      FILE *statusdst = fopen(str, "rb");
      if (statusdst)
      {
        fclose(statusdst);
        CrySetFileAttributes(str, FILE_ATTRIBUTE_ARCHIVE);
        fp = gEnv->pCryPak->FOpen(nameComb.c_str(), "r+");
      }
    }
    m_FPCacheCombinations[nType] = fp;
    Combinations = &m_ShaderCacheCombinations[nType];
  }

  int nLine = 0;
  char *pPtr = (char *)pCombinations;
  if (fp || !bFromFile)
  {
    while (!sEOF(bFromFile, pPtr, fp))
    {
      nLine++;

      str[0] = 0;
      if (bFromFile)
        gEnv->pCryPak->FGets(str, 2047, fp);
      else
        fxFillCR(&pPtr, str);
      if (!str[0])
        continue;

      if (str[0]=='/' && str[1]=='/')			// commented line e.g. // BadLine: Metal@Common_ShadowPS(%BIllum@IlluminationPS(%DIFFUSE|%ENVCMAMB|%ALPHAGLOW|%STAT_BRANCHING)(%_RT_AMBIENT|%_RT_BUMP|%_RT_GLOW)(101)(0)(0)(ps_2_0)
        continue;

      int size = strlen(str);
      if (str[size-1] == 0xa)
        str[size-1] = 0;
      SCacheCombination cmb;
      char *s = str;
      SkipCharacters(&s, kWhiteSpace);
      if (s[0] != '<')
        continue;
      if (!bForLevel)
      {
        int nVer = atoi(&s[1]);
        if (nVer != SHADER_LIST_VER)
          continue;
        if (s[2] != '>')
          continue;
        s += 3;
      }
      else
      {
        s = strchr(str, '>');
        if (!s)
          continue;
        s++;
      }
      char *st = s;
      s = strchr(s, '(');
      char name[128];
      if (s)
      {
        memcpy(name, st, s-st);
        name[s-st] = 0;
        cmb.Name = name;
        s++;
      }
      else
      {
        continue;
      }
      uint32 nHW = 0;
      cmb.nGL = sGetGL(&s, cmb.Name, nHW);
      if (cmb.nGL == -1)
      {
        const char *szFileName = nameComb.c_str();
        if (szFileName)
        {
          iLog->Log("Error: Error in '%s' file (Line: %d)", szFileName, nLine);
        }
        else
        {
          assert(!bFromFile);
          iLog->Log("Error: Error in non-file shader (Line: %d)", nLine);
        }
        sSkipLine(s);
        goto end;
      }

      s = strchr(s, '(');
      if (!s)
      {
        sSkipLine(s);
        goto end;
      }
      s++;
      cmb.nRT = sGetRT(&s);

      s = strchr(s, '(');
      if (!s)
      {
        sSkipLine(s);
        goto end;
      }
      s++;
      cmb.nLT = shGetHex(s);

      s = strchr(s, '(');
      if (!s)
      {
        sSkipLine(s);
        goto end;
      }
      s++;
      cmb.nMD = shGetHex(s);

      s = strchr(s, '(');
      if (!s)
      {
        sSkipLine(s);
        goto end;
      }
      s++;
      cmb.nMDV = shGetHex(s);

      s = strchr(s, '(');
      if (s)
      {
        s++;
        cmb.eCL = CHWShader::mfStringClass(s);
        assert (cmb.eCL < eHWSC_Max);
      }
      else
        cmb.eCL = eHWSC_Max;
      if (true || cmb.eCL < eHWSC_Max)
      {
        CCryName nm = CCryName(st);
        FXShaderCacheCombinationsItor it = Combinations->find(nm);
        if (it != Combinations->end())
        {
          //assert(false);
        }
        else
        {
          cmb.CacheName = nm;
          Combinations->insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
        }
        if (nHW)
        {
          for (int j=0; j<64; j++)
          {
            if (((uint64)1<<j) & nHW)
            {
              cmb.nGL &= ~((uint64)1<<j);
              sIterateHW_r(Combinations, cmb, j+1, nHW, name);
              cmb.nGL |= ((uint64)1<<j);
              sIterateHW_r(Combinations, cmb, j+1, nHW, name);
              cmb.nGL &= ~((uint64)1<<j);
            }
          }
        }
      }
      else
end:
        iLog->Log("Error: Error in '%s' file (Line: %d)", nameComb.c_str(), nLine);
    }
  }
}

#if !defined(XENON) && !defined(PS3) && !defined(NULL_RENDERER)
static void sResetDepend_r(SShaderGen *pGen, SShaderGenBit *pBit, SCacheCombination& cm)
{
  if (!pBit->m_DependResets.size())
    return;
  uint32 i, j;

  for (i=0; i<pBit->m_DependResets.size(); i++)
  {
    const char *c = pBit->m_DependResets[i].c_str();
    for (j=0; j<pGen->m_BitMask.Num(); j++)
    {
      SShaderGenBit *pBit1 = pGen->m_BitMask[j];
      if (!stricmp(pBit1->m_ParamName.c_str(), c))
      {
        cm.nRT &= ~pBit1->m_Mask;
        sResetDepend_r(pGen, pBit1, cm);
        break;
      }
    }
  }
}

static void sSetDepend_r(SShaderGen *pGen, SShaderGenBit *pBit, SCacheCombination& cm)
{
  if (!pBit->m_DependSets.size())
    return;
  uint32 i, j;

  for (i=0; i<pBit->m_DependSets.size(); i++)
  {
    const char *c = pBit->m_DependSets[i].c_str();
    for (j=0; j<pGen->m_BitMask.Num(); j++)
    {
      SShaderGenBit *pBit1 = pGen->m_BitMask[j];
      if (!stricmp(pBit1->m_ParamName.c_str(), c))
      {
        cm.nRT |= pBit1->m_Mask;
        sSetDepend_r(pGen, pBit1, cm);
        break;
      }
    }
  }
}

// Support for single light only
static bool sIterateDL(DWORD& dwDL)
{
  int nLights = dwDL & 0xf;
  int nType[4];
  int i;

  if (!nLights)
  {
    dwDL = 1;
    return true;
  }
  for (i=0; i<nLights; i++)
  {
    nType[i] = (dwDL >> (SLMF_LTYPE_SHIFT + (i*SLMF_LTYPE_BITS))) & ((1<<SLMF_LTYPE_BITS)-1);
  }
  switch (nLights)
  {
    case 1:
      if ((nType[0]&3)<2)
      {
        nType[0]++;
      }
      else
      {
        return false;
        nLights = 2;
        nType[0] = SLMF_DIRECT;
        nType[1] = SLMF_POINT;
      }
      break;
    case 2:
      if ((nType[0]&3) == SLMF_DIRECT)
      {
        nType[0] = SLMF_POINT;
        nType[1] = SLMF_POINT;
      }
      else
      {
        nLights = 3;
        nType[0] = SLMF_DIRECT;
        nType[1] = SLMF_POINT;
        nType[2] = SLMF_POINT;
      }
      break;
    case 3:
      if ((nType[0]&3) == SLMF_DIRECT)
      {
        nType[0] = SLMF_POINT;
        nType[1] = SLMF_POINT;
        nType[2] = SLMF_POINT;
      }
      else
      {
        nLights = 4;
        nType[0] = SLMF_DIRECT;
        nType[1] = SLMF_POINT;
        nType[2] = SLMF_POINT;
        nType[3] = SLMF_POINT;
      }
      break;
    case 4:
      if ((nType[0]&3) == SLMF_DIRECT)
      {
        nType[0] = SLMF_POINT;
        nType[1] = SLMF_POINT;
        nType[2] = SLMF_POINT;
        nType[3] = SLMF_POINT;
      }
      else
        return false;
  }
  dwDL = nLights;
  for (i=0; i<nLights; i++)
  {
    dwDL |= nType[i] << (SLMF_LTYPE_SHIFT + i*SLMF_LTYPE_BITS);
  }
  return true;
}

/*static bool sIterateDL(DWORD& dwDL)
{
  int nLights = dwDL & 0xf;
  int nType[4];
  int i;

  if (!nLights)
  {
    dwDL = 1;
    return true;
  }
  for (i=0; i<nLights; i++)
  {
    nType[i] = (dwDL >> (SLMF_LTYPE_SHIFT + (i*SLMF_LTYPE_BITS))) & ((1<<SLMF_LTYPE_BITS)-1);
  }
  switch (nLights)
  {
  case 1:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (nType[0]<2)
        nType[0]++;
      else
      {
        nLights = 2;
        nType[0] = SLMF_DIRECT;
        nType[1] = SLMF_POINT;
      }
      break;
  case 2:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (!(nType[1] & SLMF_RAE_ENABLED))
        nType[1] |= SLMF_RAE_ENABLED;
      else
        if (nType[0] == SLMF_DIRECT)
          nType[0] = SLMF_POINT;
        else
        {
          nLights = 3;
          nType[0] = SLMF_DIRECT;
          nType[1] = SLMF_POINT;
          nType[2] = SLMF_POINT;
        }
        break;
  case 3:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (!(nType[1] & SLMF_RAE_ENABLED))
        nType[1] |= SLMF_RAE_ENABLED;
      else
        if (!(nType[2] & SLMF_RAE_ENABLED))
          nType[2] |= SLMF_RAE_ENABLED;
        else
          if (nType[0] == SLMF_DIRECT)
            nType[0] = SLMF_POINT;
          else
          {
            nLights = 4;
            nType[0] = SLMF_DIRECT;
            nType[1] = SLMF_POINT;
            nType[2] = SLMF_POINT;
            nType[3] = SLMF_POINT;
          }
          break;
  case 4:
    if (!(nType[0] & SLMF_RAE_ENABLED))
      nType[0] |= SLMF_RAE_ENABLED;
    else
      if (!(nType[1] & SLMF_RAE_ENABLED))
        nType[1] |= SLMF_RAE_ENABLED;
      else
        if (!(nType[2] & SLMF_RAE_ENABLED))
          nType[2] |= SLMF_RAE_ENABLED;
        else
          if (!(nType[3] & SLMF_RAE_ENABLED))
            nType[3] |= SLMF_RAE_ENABLED;
          else
            if (nType[0] == SLMF_DIRECT)
              nType[0] = SLMF_POINT;
            else
              return false;
  }
  dwDL = nLights;
  for (i=0; i<nLights; i++)
  {
    dwDL |= nType[i] << (SLMF_LTYPE_SHIFT + i*SLMF_LTYPE_BITS);
  }
  return true;
}*/

void CShaderMan::mfAddLTCombination(SCacheCombination *cmb, FXShaderCacheCombinations& CmbsMapDst, DWORD dwL)
{
  char str[1024];
  char sLT[64];

  SCacheCombination cm;
  cm = *cmb;
  cm.nLT = dwL;

  const char *c = strchr(cmb->CacheName.c_str(), ')');
  c = strchr(&c[1], ')');
  int len = (int)(c-cmb->CacheName.c_str()+1);
  strncpy(str, cmb->CacheName.c_str(), len);
  str[len] = 0;
  strcat(str, "(");
  sprintf(sLT, "%x", (uint32)dwL);
  strcat(str, sLT);
  c = strchr(&c[2], ')');
  strcat(str, c);
  CCryName nm = CCryName(str);
  cm.CacheName = nm;
  FXShaderCacheCombinationsItor it = CmbsMapDst.find(nm);
  if (it == CmbsMapDst.end())
  {
    CmbsMapDst.insert(FXShaderCacheCombinationsItor::value_type(nm, cm));
  }
}

void CShaderMan::mfAddLTCombinations(SCacheCombination *cmb, FXShaderCacheCombinations& CmbsMapDst)
{
  if (!CRenderer::CV_r_shadersprecachealllights)
    return;

  DWORD dwL = 0;  // 0 lights
  bool bRes = false;
  do
  {
    // !workaround: Do not iterate multiple lights for low spec
    if ((cmb->nRT & (g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1])) || (dwL & 0xf)<=1)
      mfAddLTCombination(cmb, CmbsMapDst, dwL);
    bRes = sIterateDL(dwL);            	
  } while(bRes);
}


void CShaderMan::mfAddRTCombination_r(int nComb, FXShaderCacheCombinations& CmbsMapDst, SCacheCombination *cmb, CHWShader *pSH, bool bAutoPrecache)
{
  uint32 i, j, n;
  uint32 dwType = pSH->m_dwShaderType;
  if (!dwType)
    return;
  for (i=nComb; i<m_pGlobalExt->m_BitMask.Num(); i++)
  {
    SShaderGenBit *pBit = m_pGlobalExt->m_BitMask[i];
    if (bAutoPrecache && !(pBit->m_Flags & (SHGF_AUTO_PRECACHE | SHGF_LOWSPEC_AUTO_PRECACHE)))
      continue;

    // Precache this flag on low-spec only
    if (pBit->m_Flags & SHGF_LOWSPEC_AUTO_PRECACHE)
    {
      if (cmb->nRT & (g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1]))
        continue;
    }
    for (n=0; n<pBit->m_PrecacheNames.size(); n++)
    {
      if (pBit->m_PrecacheNames[n] == dwType)
        break;
    }
    if (n < pBit->m_PrecacheNames.size())
    {
      SCacheCombination cm;
      cm = *cmb;
      cm.nRT &= ~pBit->m_Mask;
      cm.nRT |= (pBit->m_Mask ^ cmb->nRT) & pBit->m_Mask;
      if (!bAutoPrecache)
      {
        uint64 nBitSet = pBit->m_Mask & cmb->nRT;
        if (nBitSet)
          sSetDepend_r(m_pGlobalExt, pBit, cm);
        else
          sResetDepend_r(m_pGlobalExt, pBit, cm);
      }

      char str[1024];
      const char *c = strchr(cmb->CacheName.c_str(), '(');
      int len = (int)(c-cmb->CacheName.c_str());
      strncpy(str, cmb->CacheName.c_str(), len);
      str[len] = 0;
      const char *c1 = strchr(&c[1], '(');
      len = (int)(c1-c);
      strncat(str, c, len);
      strcat(str, "(");
      SShaderGen *pG = m_pGlobalExt;
      stack_string sRT;
      for (j=0; j<pG->m_BitMask.Num(); j++)
      {
        SShaderGenBit *pBit = pG->m_BitMask[j];
        if (pBit->m_Mask & cm.nRT)
        {
          if (!sRT.empty())
            sRT += "|";
          sRT += pBit->m_ParamName.c_str();
        }
      }
      strcat(str, sRT.c_str());
      c1 = strchr(&c1[1], ')');
      strcat(str, c1);
      CCryName nm = CCryName(str);
      cm.CacheName = nm;
      // workaround: don't allow unsupported quality mode
      uint64 nQMask = g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1];
      if ((cm.nRT & nQMask) != nQMask)
      {
        FXShaderCacheCombinationsItor it = CmbsMapDst.find(nm);
        if (it == CmbsMapDst.end())
        {
          CmbsMapDst.insert(FXShaderCacheCombinationsItor::value_type(nm, cm));
        }
      }
      if (pSH->m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING))
        mfAddLTCombinations(&cm, CmbsMapDst);
      mfAddRTCombination_r(i+1, CmbsMapDst, &cm, pSH, bAutoPrecache);
    }
  }
}

void CShaderMan::mfAddRTCombinations(FXShaderCacheCombinations& CmbsMapSrc, FXShaderCacheCombinations& CmbsMapDst, CHWShader *pSH, bool bListOnly)
{
  if (pSH->m_nFrameLoad == gRenDev->GetFrameID())
    return;
  pSH->m_nFrameLoad = gRenDev->GetFrameID();
  uint32 dwType = pSH->m_dwShaderType;
  if (!dwType)
    return;
  const char *str2 = pSH->mfGetEntryName();
  FXShaderCacheCombinationsItor itor;
  for (itor=CmbsMapSrc.begin(); itor!=CmbsMapSrc.end(); itor++)
  {
    SCacheCombination *cmb = &itor->second;
    const char *c = strchr(cmb->Name.c_str(), '@');
    if (!c)
      c = strchr(cmb->Name.c_str(), '/');
    assert(c);
    if (!c)
      continue;
    if (stricmp(&c[1], str2))
      continue;
    /*if (!stricmp(str2, "MetalReflVS"))
    {
      if (cmb->nGL == 0x1093)
      {
        int nnn = 0;
      }
    }*/
    if (bListOnly)
    {
      if (pSH->m_Flags & (HWSG_SUPPORTS_MULTILIGHTS | HWSG_SUPPORTS_LIGHTING))
        mfAddLTCombinations(cmb, CmbsMapDst);
      mfAddRTCombination_r(0, CmbsMapDst, cmb, pSH, true);
    }
    else
      mfAddRTCombination_r(0, CmbsMapDst, cmb, pSH, false);
  }
}

float fTime0;
float fTime1;
float fTime2;

/*static void sResetAutoGL_r(int i, uint32 nMask, uint32 nMaskAuto, SCacheCombination *cmb, std::vector<SCacheCombination>& Cmbs)
{
  if (!nMask)
    return;
  uint32 nRes = (cmb->nGLAuto & nMaskAuto) & ~nMask;
  if (!nRes)
    return;
  SCacheCombination cm = *cmb;
  cm.nGLAuto &= ~nMask;
  Cmbs.push_back(cm);
  for (int j=0; j<32; j++)
  {
    if (j == i)
      continue;
    uint32 nM = (1<<j) & nMaskAuto;
    if (!nM)
      continue;
    sResetAutoGL_r(j, nM, nMaskAuto, &cm, Cmbs);
  }
}

static void sSetAutoGL_r(int i, uint32 nMask, uint32 nMaskAuto, SCacheCombination *cmb, std::vector<SCacheCombination>& Cmbs)
{
  if (!nMask)
    return;
  SCacheCombination cm = *cmb;
  cm.nGLAuto |= nMask;
  Cmbs.push_back(cm);
  sResetAutoGL_r(i, nMask, nMaskAuto, &cm, Cmbs);
  for (int j=0; j<32; j++)
  {
    if (j == i)
      continue;
    uint32 nM = (1<<j) & nMaskAuto;
    if (!nM)
      continue;
    sSetAutoGL_r(j, nM, nMaskAuto, &cm, Cmbs);
  }
}*/
#endif // XENON

void CShaderMan::mfInsertNewCombination(uint64 nGL, uint64 nRT, uint32 nLT, uint32 nMD, uint32 nMDV, EHWShaderClass eCL, const char *name, int nID, string *Str, byte bStore)
{
  char str[2048];
#if !defined(PS3)
  if (!m_FPCacheCombinations[nID] && bStore)
    return;
#endif

  stack_string sGL;
  stack_string sRT;
  uint32 i, j;
  SShaderGenComb *c = NULL;
  if (nGL)
  {
    const char *m = strchr(name, '@');
    if (!m)
      m = strchr(name, '/');
    assert(m);
    char nmFX[128];
    if (m)
    {
      strncpy(nmFX, name, m-name);
      nmFX[m-name] = 0;
      c = mfGetShaderGenInfo(nmFX);
      if (c && c->pGen && c->pGen->m_BitMask.Num())
      {
        SShaderGen *pG = c->pGen;
        for (i=0; i<64; i++)
        {
          if (nGL & ((uint64)1<<i))
          {
            for (j=0; j<pG->m_BitMask.Num(); j++)
            {
              SShaderGenBit *pBit = pG->m_BitMask[j];
              if (pBit->m_Mask & (nGL & ((uint64)1<<i)))
              {
                if (!sGL.empty())
                  sGL += "|";
                sGL += pBit->m_ParamName.c_str();
                break;
              }
            }
            if (j == pG->m_BitMask.Num())
            {
              /*if (!sGL.empty())
              sGL += "|";
              char ss[32];
              sprintf(ss, "0x%x", 1<<i);
              sGL += ss;
              //assert(0);
              iLog->Log("WARNING: CShaderMan::mfInsertNewCombination: Mask 0x%x doesn't have associated global name for shader '%s'", 1<<i, c->Name);*/
            }
          }
        }
      }
    }
  }
  if (nRT)
  {
    SShaderGen *pG = m_pGlobalExt;
    if (pG)
    {
      for (i=0; i<pG->m_BitMask.Num(); i++)
      {
        SShaderGenBit *pBit = pG->m_BitMask[i];
        if (pBit->m_Mask & nRT)
        {
          if (!sRT.empty())
            sRT += "|";
          sRT += pBit->m_ParamName.c_str();
        }
      }
    }
  }

  if (bStore==1 && nLT)
    nLT = 1;
  sprintf(str, "<%d>%s(%s)(%s)(%x)(%x)(%x)(%s)", SHADER_LIST_VER, name, sGL.c_str(), sRT.c_str(), nLT, nMD, nMDV, CHWShader::mfClassString(eCL));



	if (!bStore)
  {
    if (Str)
      *Str = str;
    return;
  }
  CCryName nm;
  if (str[0] == '<' && str[2]=='>')
    nm = CCryName(&str[3]);
  else
    nm = CCryName(str);
  FXShaderCacheCombinationsItor it = m_ShaderCacheCombinations[nID].find(nm);
  if (it != m_ShaderCacheCombinations[nID].end())
    return;
  SCacheCombination cmb;
  cmb.Name = name;
  cmb.CacheName = nm;
  cmb.nGL = nGL;
  cmb.nRT = nRT;
  cmb.nLT = nLT;
  cmb.nMD = nMD;
  cmb.nMDV = nMDV;
  cmb.eCL = eCL;
  {
		stack_string nameOut;
		mfGetShaderListPath(nameOut, nID);

		static CryCriticalSection s_cResLock;
		AUTO_LOCK(s_cResLock); // Not thread safe without this

		if (m_FPCacheCombinations[nID])
		{
			m_ShaderCacheCombinations[nID].insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
			gEnv->pCryPak->FPrintf(m_FPCacheCombinations[nID], "%s\n", str);
			gEnv->pCryPak->FFlush(m_FPCacheCombinations[nID]);
		}
  }
  if (Str)
    *Str = str;
}

const char *CShaderMan::mfGetShaderCompileFlags(EHWShaderClass eClass) const
{
	// NOTE: when updating remote compiler folders, please ensure folders path is matching

  const char* pCompilerD3D9	=	"PCD3D9/v001/fxc.exe /nologo /E %s /T %s /Zpr /Gec /Fo %s %s";  
	const char* pCompilerPS3="PS3/v014/DXPSShaderCompiler.exe %s %s %s %s";
	const char* pCompilerXenon="X360/v002/fxcx.exe /nologo /E %s /T %s /Zpr /XQremoveunusedpsinputs /Fo %s %s";

  const char* pCompilerD3D11="PCD3D11/v001/fxc.exe /nologo /E %s /T %s /Zpr /Gec /Fo %s %s";
  const char* pCompilerD3D11_PC="PCD3D11/v001/fxc.exe /nologo /E %s /T %s /Zpr /Gec /Fh %s %s";

  // Certain shaders with per-vertex lighting fail to compile in d3d9 - we workaround by using "disable flow control" hint
  //if (eProfileType == eHWSP_VS_2_0) 
  //  pCompilerD3D9	=	"PCD3D9/v001/fxc.exe /nologo /E %s /T %s /Zpr /Gec /Gfa /Fo %s %s";  

	// Set debug information
	if(CRenderer::CV_r_shadersdebug==3)
		pCompilerD3D11="PCD3D11/v001/fxc.exe /nologo /E %s /T %s /Zpr /Gec /Zi /Od /Fo %s %s";



#if defined (DIRECT3D10)
	return pCompilerD3D11;
#else
	if (CParserBin::m_bD3D11)
    return pCompilerD3D11_PC;
	else if (CParserBin::m_bXenon)
		return pCompilerXenon;
	else if (CParserBin::m_bPS3)
		return pCompilerPS3;
	else
		return pCompilerD3D9;
#endif
}

inline bool sCompareComb(const SCacheCombination &a, const SCacheCombination &b)
{
  int32 dif;

  char str1[128], str2[128];
  strcpy(str1, a.Name.c_str());
  strcpy(str2, b.Name.c_str());
  char *c = strchr(str1, '@');
  if (!c)
    c = strchr(str1, '/');
  //assert(c);
  if (c)
    *c = 0;
  c = strchr(str2, '@');
  if (!c)
    c = strchr(str2, '/');
  //assert(c);
  if (c)
    *c = 0;
  dif = stricmp(str1, str2);
  if (dif != 0)
    return dif < 0;

  if (a.nGL != b.nGL)
    return a.nGL < b.nGL;

  return false;
}

#if !defined (XENON) && !defined(PS3) && !defined(NULL_RENDERER)

void CShaderMan::AddGLCombinations(CShader *pSH, std::vector<SCacheCombination>& CmbsGL)
{
  int i, j;
  uint64 nMask = 0;
  if (pSH->m_pGenShader)
  {
    SShaderGen *pG = pSH->m_pGenShader->m_ShaderGenParams;
    for (i=0; i<pG->m_BitMask.size(); i++)
    {
      SShaderGenBit *pB = pG->m_BitMask[i];
      SCacheCombination cc;
      cc.Name = pB->m_ParamName;
      for (j=0; j<m_pGlobalExt->m_BitMask.size(); j++)
      {
        SShaderGenBit *pB1 = m_pGlobalExt->m_BitMask[j];
        if (pB1->m_ParamName == pB->m_ParamName)
        {
          nMask |= pB1->m_Mask;
          break;
        }
      }
      assert(j != m_pGlobalExt->m_BitMask.size());
      for (i=0; i<64; i++)
      {
        //if (nMask & (1<<i))
        //  SetBit_r(i, CmbsGL);
      }
    }
  }
  else
  {
    SCacheCombination cc;
    cc.nGL = 0;
    CmbsGL.push_back(cc);
  }
}

void CShaderMan::AddGLCombination(FXShaderCacheCombinations& CmbsMap, SCacheCombination& cmb)
{
  char str[1024];
  const char *st = cmb.CacheName.c_str();
  if (st[0] == '<')
    st += 3;
  const char *s = strchr(st, '@');
  char name[128];
  if (!s)
    s = strchr(st, '/');
  if (s)
  {
    memcpy(name, st, s-st);
    name[s-st] = 0;
  }
  else
    strcpy(name, st);
  sprintf(str, "%s(%I64x)(%x)(%x)", name, cmb.nGL, cmb.nMD, cmb.nMDV);
  CCryName nm = CCryName(str);
  FXShaderCacheCombinationsItor it = CmbsMap.find(nm);
  if (it == CmbsMap.end())
  {
    cmb.CacheName = nm;
    cmb.Name = nm;
    CmbsMap.insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
  }
}

/*string str;
gRenDev->m_cEF.mfInsertNewCombination(cmb.nGL, cmb.nRT, cmb.nLT, cmb.nMD, cmb.nMDV, cmb.ePR, szName, 0, &str, false);
CCryName nm = CCryName(str.c_str());
FXShaderCacheCombinationsItor it = Combinations->find(nm);
if (it == Combinations->end())
{
cmb.CacheName = nm;
Combinations->insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
}
for (int j=i; j<64; j++)
{
if (((uint64)1<<j) & nHW)
{
cmb.nGL &= ~((uint64)1<<j);
sIterateHW_r(Combinations, cmb, j+1, nHW, szName);
cmb.nGL |= ((uint64)1<<j);
sIterateHW_r(Combinations, cmb, j+1, nHW, szName);
}
}*/

void CShaderMan::AddCombination(SCacheCombination& cmb,  FXShaderCacheCombinations& CmbsMap, CHWShader *pHWS)
{
  char str[2048];
  sprintf(str, "%s(%I64x)(%I64x)(%d)(%d)(%d)", cmb.Name.c_str(), cmb.nGL, cmb.nRT, cmb.nLT, cmb.nMD, cmb.nMDV);
  CCryName nm = CCryName(str);
  FXShaderCacheCombinationsItor it = CmbsMap.find(nm);
  if (it == CmbsMap.end())
  {
    cmb.CacheName = nm;
    CmbsMap.insert(FXShaderCacheCombinationsItor::value_type(nm, cmb));
  }
}

void CShaderMan::AddLTCombinations(SCacheCombination& cmb, FXShaderCacheCombinations& CmbsMap, CHWShader *pHWS)
{
  assert(pHWS->m_Flags & HWSG_SUPPORTS_LIGHTING);

  // Just single light support

  // Directional light
  cmb.nLT = 1;
  AddCombination(cmb, CmbsMap, pHWS);

  // Point light
  cmb.nLT = 0x101;
  AddCombination(cmb, CmbsMap, pHWS);

  // Projected light
  cmb.nLT = 0x201;
  AddCombination(cmb, CmbsMap, pHWS);
}

void CShaderMan::AddRTCombinations(FXShaderCacheCombinations& CmbsMap, CHWShader *pHWS, CShader *pSH, FXShaderCacheCombinations *Combinations)
{
  SCacheCombination cmb;

  uint32 nType = pHWS->m_dwShaderType;

  uint32 i, j;
  SShaderGen *pGen = m_pGlobalExt;
  int nBits = 0;

  uint32 nBitsPlatform = 0;
  if (CParserBin::m_bXenon)
    nBitsPlatform |= SHGD_HW_X360;
  else
  if (CParserBin::m_bPS3)
    nBitsPlatform |= SHGD_HW_PS3;
  else
  if (CParserBin::m_bD3D11)
    nBitsPlatform |= SHGD_HW_DX10;
  else
    nBitsPlatform |= SHGD_HW_DX9;

  uint64 BitMask[64];
  memset(BitMask, 0, sizeof(BitMask));

  // Make a mask of flags affected by this type of shader
  uint64 nRTMask = 0;
  uint64 nSetMask = 0;

  if (nType)
  {
    for (i=0; i<pGen->m_BitMask.size(); i++)
    {
      SShaderGenBit *pBit = pGen->m_BitMask[i];
      if (!pBit->m_Mask)
        continue;
      if (nBitsPlatform & pBit->m_nDependencyReset)
        continue;
      for (j=0; j<pBit->m_PrecacheNames.size(); j++)
      {
        if (pBit->m_PrecacheNames[j] == nType)
        {
          if (nBitsPlatform & pBit->m_nDependencySet)
          {
            nSetMask |= pBit->m_Mask;
            continue;
          }
          BitMask[nBits++] = pBit->m_Mask;
          nRTMask |= pBit->m_Mask;
          break;
        }
      }
    }
  }
  if (nBits > 10)
    CryLog( "WARNING: Number of runtime bits for shader '%s' - %d: exceed 10 (too many combinations will be produced)...", pHWS->GetName(), nBits);
  if (nBits > 30)
  {
    CryLog( "Error: Ignore...");
    return;
  }

  cmb.eCL = pHWS->m_eSHClass;
  string szName = string(pSH->GetName());
  if (CParserBin::m_bXenon)
    szName += string("/") + string(pHWS->m_EntryFunc.c_str());
  else
    szName += string("@") + string(pHWS->m_EntryFunc.c_str());
  cmb.Name = szName;
  cmb.nGL = pHWS->m_nMaskGenShader;

  // For unknown shader type just add combinations from the list
  if (!nType)
  {
    FXShaderCacheCombinationsItor itor;
    for (itor=Combinations->begin(); itor!=Combinations->end(); itor++)
    {
      SCacheCombination *c = &itor->second;
      if (c->Name == cmb.Name && c->nGL == pHWS->m_nMaskGenFX)
      {
        cmb = *c;
        AddCombination(cmb, CmbsMap, pHWS);
      }
    }
    return;
  }

  cmb.nLT = 0;
  cmb.nMD = 0;
  cmb.nMDV = 0;
  cmb.nRT = 0;

  int nIterations = 1<<nBits;
  for (i=0; i<nIterations; i++)
  {
    cmb.nRT = nSetMask;
    cmb.nLT = 0;
    for (j=0; j<nBits; j++)
    {
      if ((1<<j) & i)
        cmb.nRT |= BitMask[j];
    }
    /*if (cmb.nRT == 0x40002)
    {
      int nnn = 0;
    }*/
    AddCombination(cmb, CmbsMap, pHWS);
    if (pHWS->m_Flags & HWSG_SUPPORTS_LIGHTING)
      AddLTCombinations(cmb, CmbsMap, pHWS);
  }
}

void CShaderMan::_PrecacheShaderList(bool bGLOnly, bool bStatsOnly)
{
  float t0 = gEnv->pTimer->GetAsyncCurTime();

  if (!m_pGlobalExt)
    return;

  if (bGLOnly)
    m_eCacheMode = eSC_BuildGlobal;
  else
    m_eCacheMode = eSC_BuildGlobalList;

  uint32 nSaveFeatures = gRenDev->m_Features;
  int nAsync = CRenderer::CV_r_shadersasynccompiling;
  if (nAsync != 3)
    CRenderer::CV_r_shadersasynccompiling = 0;

  // Command line shaders precaching
  gRenDev->m_Features |= RFT_HW_PS20 | RFT_HW_PS2X | RFT_HW_PS30;
  m_bActivatePhase = false;
  FXShaderCacheCombinations *Combinations = &m_ShaderCacheCombinations[0];
  FXShaderCacheCombinationsItor itor;

  std::vector<SCacheCombination> Cmbs;
  std::vector<SCacheCombination> CmbsRT;
  FXShaderCacheCombinations CmbsMap;
  uint32 i;
  uint32 j, jj;
  uint32 n, m;
  j = 0;
  char str1[128], str2[128];

  // Extract global combinations only (including MD and MDV)
  if (bGLOnly)
  {
    for (itor=Combinations->begin(); itor!=Combinations->end(); itor++)
    {
      SCacheCombination *cmb = &itor->second;
      char str[256];
      if (!CParserBin::m_bXenon)
      {
        const char *s = strchr(cmb->Name.c_str(), '/');
        if (s)
        {
          strcpy(str, cmb->Name.c_str());
          str[s-cmb->Name.c_str()] = '@';
          cmb->Name = str;
        }
      }
      else
      {
        const char *s = strchr(cmb->Name.c_str(), '@');
        if (s)
        {
          strcpy(str, cmb->Name.c_str());
          str[s-cmb->Name.c_str()] = '/';
          cmb->Name = str;
        }
      }
      SCacheCombination cc = *cmb;
      cc.nLT = 0;
      //cc.nMD = 0;
      //cc.nMDV = 0;
      if (cc.nMD == 0x300)
      {
        int nnn = 0;
      }

      cc.nRT = 0;
      AddGLCombination(CmbsMap, cc);
    }
  }
  else
  for (itor=Combinations->begin(); itor!=Combinations->end(); itor++)
  {
    SCacheCombination *cmb = &itor->second;
    FXShaderCacheCombinationsItor it = CmbsMap.find(cmb->CacheName);
    if (it == CmbsMap.end())
    {
      CmbsMap.insert(FXShaderCacheCombinationsItor::value_type(cmb->CacheName, *cmb));
    }
  }
  for (itor=CmbsMap.begin(); itor!=CmbsMap.end(); itor++)
  {
    SCacheCombination *cmb = &itor->second;
    Cmbs.push_back(*cmb);
  }
  int nEmpty = 0;
  int nProcessed = 0;
  int nCompiled = 0;
  int nMaterialCombinations = 0;

  if (Cmbs.size() >= 1)
  {
    std::stable_sort(Cmbs.begin(), Cmbs.end(), sCompareComb);

    if (bGLOnly)
      CryLog( " \n *** %d global shader combinations found", Cmbs.size());
    nMaterialCombinations = Cmbs.size();

    m_nCombinationsProcess = Cmbs.size();
    m_bReload = true;
    m_nCombinationsCompiled = 0;
    m_nCombinationsEmpty = 0;
    uint64 nGLLast = -1;
    for (i=0; i<Cmbs.size(); i++)
    {
      SCacheCombination *cmb = &Cmbs[i];
      strcpy(str1, cmb->Name.c_str());
      char *c = strchr(str1, '@');
      if (!c)
        c = strchr(str1, '/');
      //assert(c);
      if (c)
      {
        *c = 0;
        m_szShaderPrecache = &c[1];
      }
      else
      {
        c = strchr(str1, '(');
        if (c)
        {
          *c = 0;
          m_szShaderPrecache = "";
        }
      }
      gRenDev->m_RP.m_FlagsShader_RT = 0;
      gRenDev->m_RP.m_FlagsShader_LT = 0;
      gRenDev->m_RP.m_FlagsShader_MD = 0;
      gRenDev->m_RP.m_FlagsShader_MDV = 0;
      CShader *pSH = NULL;
//       if (!stricmp(str1, "Illum") && cmb->nGL == 0x80800)
//       {
//         int nnn = 0;
//       }
      pSH = CShaderMan::mfForName(str1, 0, NULL, cmb->nGL);
      gRenDev->m_RP.m_pShader = pSH;
			assert(gRenDev->m_RP.m_pShader != 0);

      std::vector<SCacheCombination> *pCmbs = &Cmbs;
      FXShaderCacheCombinations CmbsMapRTSrc;
      FXShaderCacheCombinations CmbsMapRTDst;

      for (m=0; m<pSH->m_HWTechniques.Num(); m++)
      {
        SShaderTechnique *pTech = pSH->m_HWTechniques[m];
        for (n=0; n<pTech->m_Passes.Num(); n++)
        {
          SShaderPass *pPass = &pTech->m_Passes[n];
          if (pPass->m_PShader)
            pPass->m_PShader->m_nFrameLoad = -10;
          if (pPass->m_VShader)
            pPass->m_VShader->m_nFrameLoad = -10;
        }
      }

      if (bGLOnly)
      {
        CryLog( " \n %d -- Preparing run-time combinations for shader '%s' (0x%I64x)...", i, pSH->GetName(), pSH->m_nMaskGenFX);
        for (m=0; m<pSH->m_HWTechniques.Num(); m++)
        {
          SShaderTechnique *pTech = pSH->m_HWTechniques[m];
          for (n=0; n<pTech->m_Passes.Num(); n++)
          {
            SShaderPass *pPass = &pTech->m_Passes[n];
            if (pPass->m_PShader)
              AddRTCombinations(CmbsMapRTDst, pPass->m_PShader, pSH, Combinations);
            if (pPass->m_VShader)
              AddRTCombinations(CmbsMapRTDst, pPass->m_VShader, pSH, Combinations);
          }
        }
      }
      else
      {
        for (j=i; j<Cmbs.size(); j++)
        {
          SCacheCombination *cmba = &Cmbs[j];
          strcpy(str2, cmba->Name.c_str());
          c = strchr(str2, '@');
          if (!c)
            c = strchr(str2, '/');
          assert(c);
          if (c)
            *c = 0;
          if (stricmp(str1, str2) || cmb->nGL!=cmba->nGL)
            break;
          CmbsMapRTSrc.insert(FXShaderCacheCombinationsItor::value_type(cmba->CacheName, *cmba));
        }
        m_nCombinationsProcess -= CmbsMapRTSrc.size();

        for (itor=CmbsMapRTSrc.begin(); itor!=CmbsMapRTSrc.end(); itor++)
        {
          SCacheCombination *cmb = &itor->second;
          strcpy(str2, cmb->Name.c_str());
          FXShaderCacheCombinationsItor it = CmbsMapRTDst.find(cmb->CacheName);
          if (it == CmbsMapRTDst.end())
          {
            CmbsMapRTDst.insert(FXShaderCacheCombinationsItor::value_type(cmb->CacheName, *cmb));
          }
        }

        for (m=0; m<pSH->m_HWTechniques.Num(); m++)
        {
          SShaderTechnique *pTech = pSH->m_HWTechniques[m];
          for (n=0; n<pTech->m_Passes.Num(); n++)
          {
            SShaderPass *pPass = &pTech->m_Passes[n];
            if (pPass->m_PShader)
              mfAddRTCombinations(CmbsMapRTSrc, CmbsMapRTDst, pPass->m_PShader, true);
            if (pPass->m_VShader)
              mfAddRTCombinations(CmbsMapRTSrc, CmbsMapRTDst, pPass->m_VShader, true);
          }
        }
      }

      CmbsRT.clear();
      for (itor=CmbsMapRTDst.begin(); itor!=CmbsMapRTDst.end(); itor++)
      {
        SCacheCombination *cmb = &itor->second;
        CmbsRT.push_back(*cmb);
      }
      pCmbs = &CmbsRT;
      if (bGLOnly)
        CryLog( "*** %d run-time combinations generated...", CmbsRT.size());
      m_nCombinationsProcessOverall = CmbsRT.size();
      m_nCombinationsProcess = 0;

      CmbsMapRTDst.clear();
      CmbsMapRTSrc.clear();
      uint32 nFlags = HWSF_PRECACHE;
      if (bStatsOnly)
        nFlags |= HWSF_FAKE;
      for (jj=0; jj<pCmbs->size(); jj++)
      {
        m_nCombinationsProcess++;
        SCacheCombination *cmba = &(*pCmbs)[jj];
        c = (char *)strchr(cmba->Name.c_str(), '@');
        if (!c)
          c = (char *)strchr(cmba->Name.c_str(), '/');
        assert(c);
        if (!c)
          continue;
        m_szShaderPrecache = &c[1];
        if (!stricmp(m_szShaderPrecache, "IlluminationVS") && cmba->nRT == 0x10050000)
        {
          int nnn = 0;
        }
        //if (!stricmp(pSH->GetName(), "ParticlesNoMat"))*/
        /*if (!stricmp(cmba->CacheName.c_str(), "Cloth@Common_SG_VS(%STAT_BRANCHING|%WIND_BENDING)(%_RT_ALPHATEST|%_RT_QUALITY1|%_RT_INSTANCING_ROT|%_RT_INSTANCING_ATTR|%_RT_PSM|%_RT_HW_PCF_COMPARE)(0)(0)(800)(vs_2_0)"))
        {
        int nnn = 0;
        }*/
        /*if ((cmba->nRT == 0x10902200402)) // && cmba->nLT == 0)
        {
        int nnn = 0;
        }
        }*/
        for (m=0; m<pSH->m_HWTechniques.Num(); m++)
        {
          SShaderTechnique *pTech = pSH->m_HWTechniques[m];
          for (n=0; n<pTech->m_Passes.Num(); n++)
          {
            SShaderPass *pPass = &pTech->m_Passes[n];
            gRenDev->m_RP.m_FlagsShader_RT = cmba->nRT;
            gRenDev->m_RP.m_FlagsShader_LT = cmba->nLT;
            gRenDev->m_RP.m_FlagsShader_MD = cmba->nMD;
            gRenDev->m_RP.m_FlagsShader_MDV = cmba->nMDV;
            // Adjust some flags for low spec
            if (pPass->m_PShader)
            {
              if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, pPass->m_PShader->m_EntryFunc.c_str()) != 0)
              {
                /*if (!stricmp(m_szShaderPrecache, "Common_ZPassPS"))
                {
                  int nnn = 0;
                }*/
                uint64 nFlagsOrigShader_RT = gRenDev->m_RP.m_FlagsShader_RT & pPass->m_PShader->m_nMaskAnd_RT | pPass->m_PShader->m_nMaskOr_RT;
                uint64 nFlagsOrigShader_GL = pPass->m_PShader->m_nMaskGenShader;
                uint32 nFlagsOrigShader_LT = gRenDev->m_RP.m_FlagsShader_LT;
                if (bGLOnly)
                {
                  if (!(pPass->m_PShader->m_Flags & HWSG_FP_EMULATION))
                    pPass->m_PShader->mfModifyFlags(pSH);
                  gRenDev->m_RP.m_FlagsShader_MD = cmb->nMD;
                  gRenDev->m_RP.m_FlagsShader_MDV = cmb->nMDV;
                }
                pPass->m_PShader->mfSetV(nFlags);
                if (nFlagsOrigShader_RT != gRenDev->m_RP.m_FlagsShader_RT || nFlagsOrigShader_GL != pPass->m_PShader->m_nMaskGenShader || nFlagsOrigShader_LT != gRenDev->m_RP.m_FlagsShader_LT)
                {
                  m_nCombinationsEmpty++;
                  if (!bStatsOnly)
                    pPass->m_PShader->mfAddEmptyCombination(pSH, nFlagsOrigShader_RT, nFlagsOrigShader_GL, nFlagsOrigShader_LT);
                  pPass->m_PShader->m_nMaskGenShader = nFlagsOrigShader_GL;
                }
              }
            }
            if (pPass->m_VShader)
            {
              if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, pPass->m_VShader->m_EntryFunc.c_str()) != 0)
              {
                uint64 nFlagsOrigShader_RT = gRenDev->m_RP.m_FlagsShader_RT & pPass->m_VShader->m_nMaskAnd_RT | pPass->m_VShader->m_nMaskOr_RT;
                uint64 nFlagsOrigShader_GL = pPass->m_VShader->m_nMaskGenShader;
                uint32 nFlagsOrigShader_LT = gRenDev->m_RP.m_FlagsShader_LT;
                if (bGLOnly)
                {
                  if (!(pPass->m_VShader->m_Flags & HWSG_FP_EMULATION))
                    pPass->m_VShader->mfModifyFlags(pSH);
                  /*if (!stricmp(m_szShaderPrecache, "TerrainVS") && pSH->m_nMaskGenFX == 0x200000000) // && (nFlagsOrigShader_RT == 0x40400 || gRenDev->m_RP.m_FlagsShader_RT == 0x40400))
                  {
                    int nnn = 0;
                  }*/
                  gRenDev->m_RP.m_FlagsShader_MD = cmb->nMD;
                  gRenDev->m_RP.m_FlagsShader_MDV = cmb->nMDV;
                }
                pPass->m_VShader->mfSetV(nFlags);
                if (nFlagsOrigShader_RT != gRenDev->m_RP.m_FlagsShader_RT || nFlagsOrigShader_GL != pPass->m_VShader->m_nMaskGenShader || nFlagsOrigShader_LT != gRenDev->m_RP.m_FlagsShader_LT)
                {
                  m_nCombinationsEmpty++;
                  if (!bStatsOnly)
                    pPass->m_VShader->mfAddEmptyCombination(pSH, nFlagsOrigShader_RT, nFlagsOrigShader_GL, nFlagsOrigShader_LT);
                  pPass->m_VShader->m_nMaskGenShader = nFlagsOrigShader_GL;
                }
              }
            }
            if (pPass->m_GShader)
            {
              if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, pPass->m_GShader->m_EntryFunc.c_str()) != 0)
              {
                pPass->m_GShader->mfSetV(nFlags);
              }
            }
            if (bStatsOnly)
            {
              static int nLastCombs = 0;
              if (m_nCombinationsCompiled != nLastCombs && !(m_nCombinationsCompiled & 0x7f))
              {
                nLastCombs = m_nCombinationsCompiled;
                CryLog( "-- Processed: %d, Compiled: %d, Referenced (Empty): %d...", m_nCombinationsProcess, m_nCombinationsCompiled, m_nCombinationsEmpty);
              }
            }
#ifdef WIN32
            if (!m_bActivatePhase)
            {
              MSG msg;
              while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
              {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
              }
            }
#endif
          }
        }
        pSH->mfFlushPendedShaders();
        iLog->Update();
      }
      if (bGLOnly)
        CryLog( "-- Processed: %d, Compiled: %d, Referenced (Empty): %d...", m_nCombinationsProcess, m_nCombinationsCompiled, m_nCombinationsEmpty);

      pSH->mfFlushCache();
      CHWShader::mfFlushPendedShadersWait(256);
      if (!m_bActivatePhase)
      {
        SAFE_RELEASE(pSH);
      }
      if (!bGLOnly)
        i = j-1;

      nProcessed += m_nCombinationsProcess;
      nCompiled += m_nCombinationsCompiled;
      nEmpty += m_nCombinationsEmpty;

      m_nCombinationsProcess = 0;
      m_nCombinationsCompiled = 0;
      m_nCombinationsEmpty = 0;
    }
  }
  CHWShader::mfFlushPendedShadersWait(-1);

  // Optimise shader resources
  FXShaderCacheNamesItor it;
  n = 0;
  SOptimiseStats Stats;
  for (it=CHWShader::m_ShaderCacheList.begin(); it!=CHWShader::m_ShaderCacheList.end(); it++)
  {
    const char *szName = it->first.c_str();
    SShaderCache *c = CHWShader::mfInitCache(szName, NULL, false, it->second, !CRenderer::CV_r_shadersuserfolder, true);
    if (c)
    {
      SOptimiseStats _Stats;
      CHWShader::mfOptimiseCacheFile(c, false, &_Stats);
      Stats.nEntries += _Stats.nEntries;
      Stats.nUniqueEntries += _Stats.nUniqueEntries;
      Stats.nSizeCompressed += _Stats.nSizeCompressed;
      Stats.nSizeUncompressed += _Stats.nSizeUncompressed;
      Stats.nTokenDataSize += _Stats.nTokenDataSize;
    }
    c->Release();
    n++;
  }
  /*FILE *FP = gEnv->pCryPak->FOpen("Shaders/Cache/ShaderListDone", "w");
  if (FP)
  {
    gEnv->pCryPak->FPrintf(FP, "done: %d", m_nCombinationsProcess);
    gEnv->pCryPak->FClose(FP);
  }*/
  CHWShader::m_ShaderCacheList.clear();

  m_eCacheMode = eSC_Normal;
  m_bReload = false;
  m_szShaderPrecache = NULL;
  m_bActivatePhase = false;
  CRenderer::CV_r_shadersasynccompiling = nAsync;

  gRenDev->m_Features = nSaveFeatures;

  float t1 = gEnv->pTimer->GetAsyncCurTime();
  CryLogAlways("All shaders combinations compiled in %.2f seconds",(t1-t0) );
  CryLogAlways("Combinations: (Material: %d, Processed: %d; Compiled: %d; Removed: %d)", nMaterialCombinations, nProcessed, nCompiled, nEmpty);
  CryLogAlways("-- Shader cache overall stats: Entries: %d, Unique Entries: %d, Size: %d, Compressed Size: %d, Token data size: %d", Stats.nEntries, Stats.nUniqueEntries, Stats.nSizeUncompressed, Stats.nSizeCompressed, Stats.nTokenDataSize);

  m_nCombinationsProcess = -1;
  m_nCombinationsCompiled = -1;
  m_nCombinationsEmpty = -1;
}
#endif

void CShaderMan::mfPreactivateShaders(FXShaderCacheCombinations *Combinations)
{
  CHWShader::mfFlushPendedShadersWait(-1);
  m_eCacheMode = eSC_Preactivate;

  if (Combinations)
  {
    // Preactivation

    float t0 = gEnv->pTimer->GetAsyncCurTime();

    if (!m_pGlobalExt)
      return;

    uint32 nSaveFeatures = gRenDev->m_Features;
    int nAsync = CRenderer::CV_r_shadersasynccompiling;
    if (nAsync != 3)
      CRenderer::CV_r_shadersasynccompiling = 0;

    // Shaders preactivating phase
    m_bActivatePhase = true;

    std::vector<SCacheCombination> Cmbs;
    std::vector<SCacheCombination> CmbsRT;
    FXShaderCacheCombinations CmbsMap;
    uint32 i;
	  uint32 j, jj;
    uint32 n, m;
	  j = 0;
    char str1[128], str2[128];

    FXShaderCacheCombinationsItor itor;
    for (itor=Combinations->begin(); itor!=Combinations->end(); itor++)
    {
      SCacheCombination *cmb = &itor->second;
      FXShaderCacheCombinationsItor it = CmbsMap.find(cmb->CacheName);
      if (it == CmbsMap.end())
      {
        CmbsMap.insert(FXShaderCacheCombinationsItor::value_type(cmb->CacheName, *cmb));
      }
    }
    for (itor=CmbsMap.begin(); itor!=CmbsMap.end(); itor++)
    {
      SCacheCombination *cmb = &itor->second;
      Cmbs.push_back(*cmb);
    }
    if (Cmbs.size() > 2)
    {
      std::sort(Cmbs.begin(), Cmbs.end(), sCompareComb);

      m_nCombinationsProcess = Cmbs.size();
      m_bReload = true;
      uint64 nGLLast = -1;
      for (i=0; i<Cmbs.size(); i++)
      {
        SCacheCombination *cmb = &Cmbs[i];
        strcpy(str1, cmb->Name.c_str());
        char *c = strchr(str1, '@');
        if (!c)
          c = strchr(str1, '/');
        assert(c);
        if (c)
          *c = 0;
        m_szShaderPrecache = &c[1];
        gRenDev->m_RP.m_FlagsShader_RT = 0;
        gRenDev->m_RP.m_FlagsShader_LT = 0;
        gRenDev->m_RP.m_FlagsShader_MD = 0;
        gRenDev->m_RP.m_FlagsShader_MDV = 0;
        CShader *pSH = NULL;

        // Check if this shader already loaded
        char str3[128];
        strcpy(str3, str1);
        if (CParserBin::m_bD3D11)
          strcat(str3, "(DX)");
        else
        if (CParserBin::m_bXenon)
          strcat(str3, "(X)");
        else
        if (CParserBin::m_bPS3)
          strcat(str3, "(PS3)");
        CBaseResource *pBR = CBaseResource::GetResource(CShader::mfGetClassName(), str3, false);
        bool bGenModified = false;
        pSH = (CShader *)pBR;
        if (pSH && pSH->m_ShaderGenParams)
        {
          uint64 nGen = cmb->nGL;
          uint64 nGenHW = 0;
          /*if (nGen == 0x401093)
          {
          int nnn = 0;
          }*/
          mfModifyGenFlags(pSH, NULL, nGen, nGenHW);
          bGenModified = true;
          char nameNew[256];



          sprintf(nameNew, "%s(%I64x)", str3, nGen);

          pBR = CBaseResource::GetResource(CShader::mfGetClassName(), nameNew, false);
          pSH = (CShader *)pBR;
          nGen |= nGenHW;
        }
        //assert(pSH);
        if (!pSH)
        {
          if (cmb->nGL != nGLLast)
          {
            nGLLast = cmb->nGL;
            if (CRenderer::CV_r_shadersdebug)
            {



              iLog->LogError("Error: shader '%s (0x%I64x)' is not existing during precache phase", str1, cmb->nGL);

            }
          }
          continue;
        }
        gRenDev->m_RP.m_pShader = pSH;
				assert(gRenDev->m_RP.m_pShader != 0);

        std::vector<SCacheCombination> *pCmbs = &Cmbs;

        CmbsRT.clear();
        for (j=i; j<(int)Cmbs.size(); j++)
        {
          SCacheCombination *cmba = &Cmbs[j];
          strcpy(str2, cmba->Name.c_str());
          c = strchr(str2, '@');
          if (!c)
            c = strchr(str2, '/');
          assert(c);
          if (c)
            *c = 0;
          if (stricmp(str1, str2) || cmb->nGL!=cmba->nGL)
            break;
          CmbsRT.push_back(Cmbs[j]);
        }
        pCmbs = &CmbsRT;

        for (jj=0; jj<pCmbs->size(); jj++)
        {
          SCacheCombination *cmba = &(*pCmbs)[jj];
          c = (char *)strchr(cmba->Name.c_str(), '@');
          if (!c)
            c = (char *)strchr(cmba->Name.c_str(), '/');
          assert(c);
          if (!c)
            continue;
          m_szShaderPrecache = &c[1];
          /*if (!stricmp(m_szShaderPrecache, "IlluminationPS"))
          {
          //if (!stricmp(pSH->GetName(), "ParticlesNoMat"))*/
          /*if (!stricmp(cmba->CacheName.c_str(), "Cloth@Common_SG_VS(%STAT_BRANCHING|%WIND_BENDING)(%_RT_ALPHATEST|%_RT_QUALITY1|%_RT_INSTANCING_ROT|%_RT_INSTANCING_ATTR|%_RT_PSM|%_RT_HW_PCF_COMPARE)(0)(0)(800)(vs_2_0)"))
          {
            int nnn = 0;
          }*/
          /*if ((cmba->nRT == 0x10902200402)) // && cmba->nLT == 0)
          {
          int nnn = 0;
          }
          }*/
          for (m=0; m<pSH->m_HWTechniques.Num(); m++)
          {
            SShaderTechnique *pTech = pSH->m_HWTechniques[m];
            for (n=0; n<pTech->m_Passes.Num(); n++)
            {
              SShaderPass *pPass = &pTech->m_Passes[n];
              gRenDev->m_RP.m_FlagsShader_RT = cmba->nRT;
              gRenDev->m_RP.m_FlagsShader_LT = cmba->nLT;
              gRenDev->m_RP.m_FlagsShader_MD = cmba->nMD;
              gRenDev->m_RP.m_FlagsShader_MDV = cmba->nMDV;
              // Adjust some flags for low spec
              gRenDev->FX_ApplyShaderQuality(pSH->m_eShaderType);
              if (pPass->m_PShader && !(pPass->m_PShader->m_Flags & HWSG_FP_EMULATION))
              {
                if (gRenDev->EF_GetShaderQuality(pSH->m_eShaderType) == eSQ_Low && (gRenDev->m_RP.m_FlagsShader_LT & 0xf) > 1)
                  gRenDev->m_RP.m_FlagsShader_LT = (gRenDev->m_RP.m_FlagsShader_LT & 0xf0) | 1;
              }
              if (pPass->m_PShader)
              {
                if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, pPass->m_PShader->m_EntryFunc.c_str()) != 0)
                {
                  bool bRes = pPass->m_PShader->mfSetV(HWSF_PRECACHE);
                }
              }
              if (pPass->m_VShader)
              {
                if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, pPass->m_VShader->m_EntryFunc.c_str()) != 0)
                {
                  bool bRes = pPass->m_VShader->mfSetV(HWSF_PRECACHE);
                }
              }
              if (pPass->m_GShader)
              {
                if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, pPass->m_GShader->m_EntryFunc.c_str()) != 0)
                {
                  bool bRes = pPass->m_GShader->mfSetV(HWSF_PRECACHE);
                }
              }
            }
          }
  #if !defined(XENON) && !defined(PS3)
          pSH->mfFlushPendedShaders();
          iLog->Update();
  #endif
        }
  #if !defined(XENON) && !defined(PS3) && !defined(NULL_RENDERER)
        pSH->mfFlushCache();
  #endif
        CHWShader::mfFlushPendedShadersWait(256);
        i = j-1;
      }
    }
    CHWShader::mfFlushPendedShadersWait(-1);

    CHWShader::m_ShaderCacheList.clear();

    m_eCacheMode = eSC_Normal;
    m_nCombinationsProcess = -1;
    m_nCombinationsProcessOverall = -1;
    m_bReload = false;
    m_szShaderPrecache = NULL;
    m_bActivatePhase = false;
    CRenderer::CV_r_shadersasynccompiling = nAsync;

    gRenDev->m_Features = nSaveFeatures;

    float t1 = gEnv->pTimer->GetAsyncCurTime();
    CryLogAlways( "All shaders combinations activated in %.2f seconds",(t1-t0));
  }
}

void CHWShader::mfGenName(uint64 GLMask, uint64 RTMask, uint32 LightMask, uint32 MDMask, uint32 MDVMask, EHWShaderClass eClass, char *dstname, int nSize, byte bType)
{
  assert(dstname);
  dstname[0] = 0;

  char str[32];
  if (bType!=0 && GLMask)
  {
#if defined(__GNUC__)
    sprintf(str, "(GL%llx)", GLMask);
#else
    sprintf(str, "(GL%I64x)", GLMask);
#endif
    strncat(dstname, str, nSize);
  }
  if (bType != 0)
  {
#if defined(__GNUC__)
    sprintf(str, "(RT%llx)", RTMask);
#else
    sprintf(str, "(RT%I64x)", RTMask);
#endif
    strncat(dstname, str, nSize);
  }




  if (bType!=0 && LightMask)
  {
    sprintf(str, "(LT%x)", LightMask);
    strncat(dstname, str, nSize);
  }
  if (bType!=0 && MDMask)
  {
    sprintf(str, "(MD%x)", MDMask);
    strncat(dstname, str, nSize);
  }
  if (bType!=0 && MDVMask)
  {
    sprintf(str, "(MDV%x)", MDVMask);
    strncat(dstname, str, nSize);
  }
  if (bType!=0) // && !CParserBin::m_bD3D11 && !CParserBin::m_bXenon && !CParserBin::m_bPS3)
  {
    sprintf(str, "(%s)", mfClassString(eClass));
    strncat(dstname, str, nSize);
  }
}

#if !defined (XENON) && !defined(PS3) && !defined(NULL_RENDERER)

void CShaderMan::mfPrecacheShaders(bool bListOnly, bool bStatsOnly)
{
  CHWShader::mfFlushPendedShadersWait(-1);

  if (CRenderer::CV_r_shadersxenon)
  {
    uint32 nComp = CRenderer::CV_r_shadersintcompiler;
    CRenderer::CV_r_shadersintcompiler = 0;
		if (CRenderer::CV_r_shadersremotecompiler)
			CRenderer::CV_r_shadersintcompiler = 1;
		else
			CRenderer::CV_r_shadersintcompiler = 0;
    int nUsePOM = CRenderer::CV_r_usepom;
    CRenderer::CV_r_usepom = 0;
    gRenDev->m_bDeviceSupportsFP16Filter = true;
    gRenDev->m_bDeviceSupportsG16R16Filter = true;
    gRenDev->m_bDeviceSupportsFP16Separate = false;
    CRenderer::CV_r_shadersdynamicbranching = 0;
    gRenDev->m_Features |= RFT_HW_PS30;

    CParserBin::SetupForXenon();
    CryLogAlways("\nStarting shader compilation for X360...");
    mfInitShadersList(NULL);
    if (bListOnly)
      _PrecacheShaderList(false, bStatsOnly);
    else
      _PrecacheShaderList(true, bStatsOnly);

    _SetVar("r_ShadersXenon", 0);
    CRenderer::CV_r_shadersintcompiler = nComp;
    CRenderer::CV_r_usepom = nUsePOM;
  }
  else
  if (CRenderer::CV_r_shadersps3)
  {
    uint32 nComp = CRenderer::CV_r_shadersintcompiler;
    CRenderer::CV_r_shadersintcompiler = 0;
		if (CRenderer::CV_r_shadersremotecompiler)
			CRenderer::CV_r_shadersintcompiler = 1;
		else
			CRenderer::CV_r_shadersintcompiler = 0;
    CRenderer::CV_r_usepom = 0;
    gRenDev->m_bDeviceSupportsFP16Filter = true;
    gRenDev->m_bDeviceSupportsG16R16Filter = true;
    gRenDev->m_bDeviceSupportsFP16Separate = false;
    CRenderer::CV_r_shadersdynamicbranching = 0;
    gRenDev->m_Features |= RFT_HW_PS30;

    CParserBin::SetupForPS3();
    CryLogAlways("\nStarting shader compilation for PS3...");
    mfInitShadersList(NULL);
    if (bListOnly)
      _PrecacheShaderList(false, bStatsOnly);
    else
      _PrecacheShaderList(true, bStatsOnly);

    _SetVar("r_ShadersPS3", 0);
    CRenderer::CV_r_shadersintcompiler = nComp;
  }
  else
  if (CRenderer::CV_r_shadersdx9 || CRenderer::CV_r_shadersdx11)
  {
    uint32 nComp = CRenderer::CV_r_shadersintcompiler;
    if (CRenderer::CV_r_shadersremotecompiler)
      CRenderer::CV_r_shadersintcompiler = 1;
    else
      CRenderer::CV_r_shadersintcompiler = 0;

    if (CRenderer::CV_r_shadersdx11)
    {
      CParserBin::SetupForD3D10();
      CryLogAlways("\nStarting shader compilation for D3D11...");
      mfInitShadersList(NULL);
      if (bListOnly)
        _PrecacheShaderList(false, bStatsOnly);
      else
        _PrecacheShaderList(true, bStatsOnly);
    }

    CRenderer::CV_r_shadersintcompiler = nComp;
    if (CRenderer::CV_r_shadersremotecompiler)
      CRenderer::CV_r_shadersintcompiler = 1;

    if (CRenderer::CV_r_shadersdx9)
    {
      CParserBin::SetupForD3D9();
      CryLogAlways("Starting shader compilation for D3D9...");
      mfInitShadersList(NULL);
      if (bListOnly)
        _PrecacheShaderList(false, bStatsOnly);
      else
        _PrecacheShaderList(true, bStatsOnly);
    }

    CRenderer::CV_r_shadersintcompiler = nComp;
  }
 #if defined (DIRECT3D10)
  CParserBin::SetupForD3D10();
 #elif defined (DIRECT3D9)
  CParserBin::SetupForD3D9();
 #endif

  gRenDev->m_cEF.m_Bin.InvalidateCache();
}


void CShaderMan::mfGetLevelShadersList(std::vector<ShadersLevel>& ShaderList, const char *szDir)
{
  struct _finddata_t fileinfo;
  intptr_t handle;

  char dirn[256];
  strcpy_s(dirn, szDir);
  strcat(dirn, "*.*");

  char pkdirn[256];
  strcpy_s(pkdirn, szDir);
  strcat(pkdirn, "level.pak");
  gEnv->pCryPak->OpenPacks(pkdirn, (unsigned)0);
  handle = gEnv->pCryPak->FindFirst (dirn, &fileinfo);
  if (handle == -1)
    return;
  do
  {
    if (fileinfo.name[0] == '.')
      continue;
    if (fileinfo.attrib & _A_SUBDIR)
    {
      char ddd[512];
      sprintf(ddd, "%s%s/", szDir, fileinfo.name);

      mfGetLevelShadersList(ShaderList, ddd);
      continue;
    }
    if (stricmp(fileinfo.name, "shaderslist.txt"))
      continue;
    string s = szDir;
    s += fileinfo.name;
    FILE *fp = gEnv->pCryPak->FOpen(s.c_str(), "rb");
    if (fp)
    {
      int nSize = gEnv->pCryPak->FGetSize(fp);
      ShadersLevel sl;
      sl.szPath = szDir;
      sl.szContent.resize(nSize);
      gEnv->pCryPak->FReadRaw((void *)&sl.szContent[0], nSize, 1, fp);
      gEnv->pCryPak->FClose(fp);
      sl.szContent += '\0';
      ShaderList.push_back(sl);
    }
  } while (gEnv->pCryPak->FindNext(handle, &fileinfo) != -1);

  gEnv->pCryPak->FindClose (handle);
  gEnv->pCryPak->ClosePacks(pkdirn, 0);
}

void CShaderMan::mfStoreLevelCache(const char *szName, const ShadersLevel& sl, std::vector<SCacheLevel>& Caches)
{
  if (!szName || !szName[0])
    return;
  uint32 i, j;
  for (i=0; i<Caches.size(); i++)
  {
    SCacheLevel& CL = Caches[i];
    if (!CL.Data.size())
      continue;
    char nameCache[512];
    strncpy(nameCache, sl.szPath, 512);
    strncat(nameCache, "ShaderCache/", 512);
    SShaderCache *pC = CL.pCache;
    char cache[256];
    strcpy(cache, pC->m_Name.c_str());
    strlwr(cache);
    const char *s = strstr(cache, "cache/");
    assert (s);
    if (!s)
      continue;
    s += sizeof("cache/")-1;
    strncat(nameCache, s, 512);
    CResFile *pRF = new CResFile(nameCache);
    if (!pRF->mfOpen(RA_READ|RA_WRITE|(CParserBin::m_bEndians ? RA_ENDIANS : 0)))
    {
      if (!pRF->mfOpen(RA_CREATE|(CParserBin::m_bEndians ? RA_ENDIANS : 0)))
        continue;
    }
    SDirEntry de;
    de.Name = CShaderMan::s_cNameHEAD;
    de.flags = RF_RES_$HEAD;
    de.size = sizeof(SShaderCacheHeader);
    SShaderCacheHeader hd;
    ZeroStruct(hd);
    float fVersion = (float)FX_CACHE_VER;
    hd.m_SizeOf = sizeof(SShaderCacheHeader);
    hd.m_MinorVer = (int)(((float)fVersion - (float)(int)fVersion)*10.1f);
    hd.m_MajorVer = (int)fVersion;
    hd.m_CRC32 = pC->m_Header[0].m_CRC32;
    hd.m_bOptimised = false;
    sprintf(hd.m_szVer, "Ver: %.1f", fVersion);
    SShaderCacheHeader hdTemp, *pHD;
    pHD = &hd;
    if (CParserBin::m_bEndians)
    {
      hdTemp = hd;
      SwapEndian(hdTemp, eBigEndian);
      pHD = &hdTemp;
    }
    de.pData = pHD;
    pRF->mfFileAdd(&de);
    for (j=0; j<CL.Data.size(); j++)
    {
      SCacheLevelData &Data = CL.Data[j];
      int nDevID = Data.nOffset;
      if (nDevID < 0)
        nDevID = -nDevID;
      de.flags = (RF_COMPRESSED | RF_COMPRESS) | (nDevID << 8);
      de.size = Data.nSizeCompressedShader+4;
      de.offset = Data.nOffset;
      de.Name = Data.Name;
      byte *pData = NULL;
      if (Data.pData)
      {
        pData = new byte[Data.nSizeCompressedShader+4];
        *(int *)pData = Data.nSizeDecompressedShader;
        memcpy(&pData[4], Data.pData, Data.nSizeCompressedShader);
        de.flags |= RF_TEMPDATA;
      }
      de.pData = pData;

      /*Decodem(Data.pData, pData, Data.nSizeCompressedShader);
      SShaderCacheHeaderItem *pIt = (SShaderCacheHeaderItem *)pData;
      SAFE_DELETE_ARRAY(pData);*/

      pRF->mfFileAdd(&de);
    }
    pRF->mfFlush();

    for (j=0; j<CL.Data.size(); j++)
    {
      SAFE_DELETE_ARRAY(CL.Data[j].pData);
    }
    CL.Data.clear();
    assert(CL.pCache);
    if (CL.pCache)
    {
      for (j=0; j<CL.nRef; j++)
      {
        CL.pCache->Release();
      }
    }
  }
  Caches.clear();
}

SCacheLevel *CShaderMan::mfOpenLevelCache(const char *szCache, const char *szName, SCacheCombination *cmb, std::vector<SCacheLevel>& Caches)
{
  char nameCache[256];
  strncpy(nameCache, m_ShadersCache, 256);
  if (cmb->eCL >= eHWSC_Max)
    return NULL;
  if (cmb->eCL == eHWSC_Vertex)
    strncat(nameCache, "CGVShaders/", 256);
  else
  if (cmb->eCL == eHWSC_Pixel)
    strncat(nameCache, "CGPShaders/", 256);
  else
    strncat(nameCache, "CGGShaders/", 256);
  strncat(nameCache, szCache, 256);
  if (CParserBin::m_bXenon)
    strncat(nameCache, "/", 256);
  else
    strncat(nameCache, "@", 256);
  strncat(nameCache, szName, 256);
  fpAddExtension(nameCache, ".fxcb");
  nameCache[255] = '\0';
  SShaderCache *pCache = CHWShader::mfInitCache(nameCache, NULL, false, 0, !CRenderer::CV_r_shadersuserfolder, true);
  if (!pCache || !pCache->m_pRes[0])
  {
    SAFE_RELEASE(pCache);
    return NULL;
  }
  uint32 i;
  for (i=0; i<Caches.size(); i++)
  {
    SCacheLevel *pC = &Caches[i];
    assert(pC);
    if (!pC)
      continue;
    if (pC->pCache->m_Name == pCache->m_Name)
      break;
  }
  if (i == Caches.size())
  {
    SCacheLevel CL;
    CL.pCache = pCache;
    Caches.push_back(CL);
  }
  Caches[i].nRef++;

  return &Caches[i];
}

void CShaderMan::_PrecacheShadersLevels()
{
  std::vector<ShadersLevel> ShaderList;
  char *dirn = "Levels/";
  mfGetLevelShadersList(ShaderList, dirn);

  uint32 i, j, n, m, l;
  for (i=0; i<ShaderList.size(); i++)
  {
    ShadersLevel& sl = ShaderList[i];
    if (!sl.szContent.size())
      continue;
    FXShaderCacheCombinations Combinations;
    gRenDev->m_cEF.mfInitShadersCache(true, &Combinations, sl.szContent);

    std::vector<SCacheCombination> Cmbs;

    FXShaderCacheCombinationsItor itor;
    for (itor=Combinations.begin(); itor!=Combinations.end(); itor++)
    {
      SCacheCombination *cmb = &itor->second;
      Cmbs.push_back(*cmb);
    }
    int32 nVersion = -1;
    if (Cmbs.size() >= 1)
    {
      std::sort(Cmbs.begin(), Cmbs.end(), sCompareComb);
      uint64 nGLLast = -1;
      char str1[256], str2[256], str3[256];
      str2[0] = 0;
      std::vector<SCacheLevel> Caches;
      int nFound = 0;
      for (j=0; j<Cmbs.size(); j++)
      {
        SCacheCombination *cmb = &Cmbs[j];
        strcpy(str1, cmb->Name.c_str());
        char *c = strchr(str1, '@');
        if (!c)
          c = strchr(str1, '/');
        assert(c);
        if (c)
          *c = 0;
        m_szShaderPrecache = &c[1];
        if (stricmp(str1, str2))
        {
          mfStoreLevelCache(str2, sl, Caches);
          strcpy(str2, str1);
        }

        int nCompile = CRenderer::CV_r_shadersnocompile;
        CRenderer::CV_r_shadersnocompile = 1;
        CShader *pSH = CShaderMan::mfForName(str1, 0, NULL, cmb->nGL);
        CRenderer::CV_r_shadersnocompile = nCompile;
        gRenDev->m_RP.m_pShader = pSH;
        std::vector<SCacheCombination *> CmbsRT;
        for (m=j; m<Cmbs.size(); m++)
        {
          SCacheCombination *cmba = &Cmbs[m];
          strcpy(str3, cmba->Name.c_str());
          c = strchr(str3, '@');
          if (!c)
            c = strchr(str3, '/');
          assert(c);
          if (c)
            *c = 0;
          if (stricmp(str1, str3) || cmb->nGL!=cmba->nGL)
            break;
          CmbsRT.push_back(cmba);
        }
        for (l=0; l<CmbsRT.size(); l++)
        {
          SCacheCombination *cmba = CmbsRT[l];
          strcpy(str3, cmba->Name.c_str());
          char *c = strchr(str3, '@');
          if (!c)
            c = strchr(str3, '/');
          assert(c);
          if (c)
            *c = 0;
          m_szShaderPrecache = &c[1];
          /*if (!stricmp(m_szShaderPrecache, "Common_SG_VS"))
          {
            int nnn = 0;
          }*/
          uint32 t, p;
          uint64 nFlags_RT = cmba->nRT;
          uint64 nFlags_GL = cmba->nGL;
          for (t=0; t<pSH->m_HWTechniques.size(); t++)
          {
            SShaderTechnique *pTech = pSH->m_HWTechniques[t];
            for (p=0; p<pTech->m_Passes.size(); p++)
            {
              SShaderPass& Pass = pTech->m_Passes[p];
              if (Pass.m_VShader)
              {
                if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, Pass.m_VShader->m_EntryFunc.c_str()))
                {
                  nFlags_RT = nFlags_RT & Pass.m_VShader->m_nMaskAnd_RT | Pass.m_VShader->m_nMaskOr_RT;
                  nFlags_GL = Pass.m_VShader->m_nMaskGenShader;
                  break;
                }
              }
              if (Pass.m_PShader)
              {
                if (!m_szShaderPrecache || !stricmp(m_szShaderPrecache, Pass.m_PShader->m_EntryFunc.c_str()))
                {
                  nFlags_RT = nFlags_RT & Pass.m_PShader->m_nMaskAnd_RT | Pass.m_PShader->m_nMaskOr_RT;
                  nFlags_GL = Pass.m_PShader->m_nMaskGenShader;
                  break;
                }
              }
            }
            if (p != pTech->m_Passes.size())
              break;
          }
          if (t == pSH->m_HWTechniques.size())
            continue;
          SCacheLevel* pCache;
          if (!strnicmp(m_szShaderPrecache, "Common_", 7))
            pCache = mfOpenLevelCache("_Shared", m_szShaderPrecache, cmba, Caches);
          else
            pCache = mfOpenLevelCache(str3, m_szShaderPrecache, cmba, Caches);
          if (!pCache)
            break;
          CResFile *rf = pCache->pCache->m_pRes[0];
          uint32 nFlags_LT = cmba->nLT;
          uint32 nFlags_MD = cmba->nMD;
          uint32 nFlags_MDV = cmba->nMDV;
          char name[128];
          CHWShader::mfGenName(nFlags_GL, nFlags_RT, nFlags_LT, nFlags_MD, nFlags_MDV, cmba->eCL, name, 128, 1);
          CCryNameCRC cn = CCryNameCRC(name);
          SDirEntry *de = rf->mfGetEntry(cn);
          if (!de)
            continue;
          nFound++;
          SCacheLevelData CD;
          CD.Name = cn;
          for (n=0; n<pCache->Data.size(); n++)
          {
            SCacheLevelData &LD = pCache->Data[n];
            if (LD.pData && LD.nOffset == de->offset)
              break;
          }
          if (n == pCache->Data.size())
          {
            CD.pData = rf->mfFileReadCompressed(de, CD.nSizeDecompressedShader, CD.nSizeCompressedShader);
            assert(CD.pData);
            if (CD.pData)
            {
              CD.nOffset = de->offset;
              pCache->Data.push_back(CD);
            }
          }
          else
          {
            SCacheLevelData &LD = pCache->Data[n];
            CD.nSizeCompressedShader = LD.nSizeCompressedShader;
            CD.nSizeDecompressedShader = LD.nSizeDecompressedShader;
            CD.nOffset = -de->offset;
            CD.pData = NULL;
            pCache->Data.push_back(CD);
          }
        }
        j = m-1;
      }
      CryLogAlways("\nPrecache shaders for path: '%s' (%d of %d found)...", sl.szPath.c_str(), nFound, Cmbs.size());
      mfStoreLevelCache(str2, sl, Caches);
    }
  }
}

struct SDirectoryEnumeratorHelper
{
	void ScanDirectoryFiles( const string &root,const string &path,const string &fileSpec,std::vector<string> &files )
	{
		string dir = PathUtil::AddSlash(root + path);

		string findFilter = PathUtil::Make(dir,fileSpec);
		ICryPak *pIPak = gEnv->pCryPak;

		_finddata_t fd;
		intptr_t fhandle;

		fhandle = pIPak->FindFirst( findFilter,&fd );
		if (fhandle != -1)
		{
			do {
				// Skip back folders.
				if (fd.name[0] == '.')
					continue;
				if (fd.attrib & _A_SUBDIR) // skip sub directories.
					continue;
				files.push_back( path + fd.name );
			} while (pIPak->FindNext( fhandle,&fd ) == 0);
			pIPak->FindClose(fhandle);
		}
	}
	void ScanDirectoryRecursive( const string &root,const string &path,const string &fileSpec,std::vector<string> &files )
	{
		bool anyFound = false;
		string dir = PathUtil::AddSlash(root + path);

		ScanDirectoryFiles( root,path,fileSpec,files );

		string findFilter = PathUtil::Make(dir,"*.*");
		ICryPak *pIPak = gEnv->pCryPak;

		// Add all directories.
		_finddata_t fd;
		intptr_t fhandle;

		fhandle = pIPak->FindFirst( findFilter,&fd );
		if (fhandle != -1)
		{
			do {
				// Skip back folders.
				if (fd.name[0] == '.')
					continue;
				if (fd.attrib & _A_SUBDIR) // skip sub directories.
				{
					ScanDirectoryRecursive( root,PathUtil::AddSlash(path)+fd.name+"/",fileSpec,files );
					continue;
				}
			} while (pIPak->FindNext( fhandle,&fd ) == 0);
			pIPak->FindClose(fhandle);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
void CShaderMan::mfPakPerLevelShaderCache()
{
	std::vector<string> levelFiles;
	std::vector<string> files;
	std::vector<char> buffer;

	SDirectoryEnumeratorHelper dirHelper;

	string sDestinationPath = PathUtil::AddSlash(PathUtil::GetGameFolder()) + "_LevelCache/";

	dirHelper.ScanDirectoryRecursive( "","Levels/","level.pak",levelFiles );

	for (int lvl = 0; lvl < (int)levelFiles.size(); lvl++)
	{
		string levelFolder = PathUtil::AddSlash( PathUtil::GetPath(levelFiles[lvl]) );

		files.resize(0);
		dirHelper.ScanDirectoryRecursive( levelFolder,"ShaderCache","*.*",files );
		if (files.empty())
			continue;

		string pakFilename = PathUtil::AddSlash(PathUtil::GetGameFolder()) + levelFolder + "LevelShaderCache.pak";

		gEnv->pCryPak->RemoveFile( pakFilename.c_str() );
		
		// Create Pak file
		{
			_smart_ptr<ICryArchive> pCryArchive = gEnv->pCryPak->OpenArchive( pakFilename.c_str(),ICryArchive::FLAGS_RELATIVE_PATHS_ONLY |ICryArchive::FLAGS_CREATE_NEW);
			if (!pCryArchive)
				continue;

			for (int i = 0; i < (int)files.size(); i++)
			{
				string filename = files[i];
				string onDiskFilename = levelFolder + filename;
				CCryFile inFile;
				if (inFile.Open( onDiskFilename.c_str(),"rb" ))
				{
					int nSize = inFile.GetLength();
					if (nSize > 0)
					{
						buffer.resize(nSize);
						inFile.ReadRaw( &buffer[0],nSize );
						pCryArchive->UpdateFile( filename.c_str(),&buffer[0],buffer.size(), ICryArchive::METHOD_STORE,0 );
					}
				}
			}
		}

		// Only in editor.
#ifdef WIN32
		int lastpos = PathUtil::RemoveSlash(levelFolder).rfind('/');
		if (lastpos != string::npos)
			levelFolder = levelFolder.substr(lastpos+1);
		string toDir = sDestinationPath + levelFolder;
		string toFile = toDir + "LevelShaderCache.pak";
		
		// If pak created copy it to the _LevelCache folder
		gEnv->pCryPak->MakeDir( toDir.c_str() );
		::CopyFile( pakFilename.c_str(),toFile.c_str(),false );
#endif
	}
}

void CShaderMan::mfPrecacheShadersLevels()
{
  m_eCacheMode = eSC_BuildPerLevel;
  if (CRenderer::CV_r_shadersxenon)
  {
    CParserBin::SetupForXenon();
    CryLogAlways("\n\n\nStarting level shaders building for X360...");
    _PrecacheShadersLevels();
  }
  else
  if (CRenderer::CV_r_shadersps3)
  {
    CParserBin::SetupForPS3();
    CryLogAlways("\n\n\nStarting level shaders building for PS3...");
    _PrecacheShadersLevels();
  }
  else
  {
    if (CRenderer::CV_r_shadersdx9)
    {
      CParserBin::SetupForD3D9();
      CryLogAlways("\n\n\nStarting level shaders building for DX9...");
      _PrecacheShadersLevels();
    }
    if (CRenderer::CV_r_shadersdx11)
    {
      CParserBin::SetupForD3D10();
      CryLogAlways("\n\n\nStarting level shaders building for DX11...");
      _PrecacheShadersLevels();
    }
  }

	// Pak Shader cache.
	mfPakPerLevelShaderCache();

#if defined (DIRECT3D10)
  CParserBin::SetupForD3D10();
#elif defined (DIRECT3D9)
  CParserBin::SetupForD3D9();
#endif
  m_eCacheMode = eSC_Normal;
}


void CShaderMan::mfOptimiseShaders(const char *szFolder, bool bForce)
{
  CHWShader::mfFlushPendedShadersWait(-1);

  float t0 = gEnv->pTimer->GetAsyncCurTime();
  SShaderCache *pCache;
  uint32 i;

  std::vector<CCryName> Names;
  mfGatherFilesList(szFolder, Names, 0, false);

  SOptimiseStats Stats;
  for (i=0; i<Names.size(); i++)
  {
    const char *szName = Names[i].c_str();
    pCache = CHWShader::mfInitCache(szName, NULL, false, 0, !CRenderer::CV_r_shadersuserfolder, false);
    if (!pCache || !pCache->m_pRes[CACHE_USER])
      continue;
    SOptimiseStats _Stats;
    CHWShader::mfOptimiseCacheFile(pCache, bForce, &_Stats);
    Stats.nEntries += _Stats.nEntries;
    Stats.nUniqueEntries += _Stats.nUniqueEntries;
    Stats.nSizeCompressed += _Stats.nSizeCompressed;
    Stats.nSizeUncompressed += _Stats.nSizeUncompressed;
    Stats.nTokenDataSize += _Stats.nTokenDataSize;
    pCache->Release();
  }

  float t1 = gEnv->pTimer->GetAsyncCurTime();
  CryLog("-- All shaders combinations optimized in %.2f seconds", t1-t0);
  CryLog("-- Shader cache overall stats: Entries: %d, Unique Entries: %d, Size: %d, Compressed Size: %d, Token data size: %d", Stats.nEntries, Stats.nUniqueEntries, Stats.nSizeUncompressed, Stats.nSizeCompressed, Stats.nTokenDataSize);
}

struct SMgData
{
  CCryNameCRC Name;
  int nSize;
  uint32 CRC;
  uint32 flags;
  byte *pData;
  int nID;
  byte bProcessed;
};

static int snCurListID;
typedef std::map<CCryNameCRC, SMgData> ShaderData;
typedef ShaderData::iterator ShaderDataItor;

static void sAddToList(SShaderCache *pCache, ShaderData& Data)
{
  uint32 i;

  CResFile *pRes = pCache->m_pRes[CACHE_USER];
  std::vector<SDirEntry> *Dir = pRes->mfGetDirectory();
  for (i=0; i<Dir->size(); i++)
  {
    SDirEntry *pDE = &(*Dir)[i];
    if (pDE->Name == CShaderMan::s_cNameHEAD)
      continue;
    ShaderDataItor it = Data.find(pDE->Name);
    if (it == Data.end())
    {
      SMgData d;
      d.nSize = pRes->mfFileRead(pDE);
      d.flags = pDE->flags;
      if (pDE->flags & RF_RES_$)
      {
        d.pData = new byte[d.nSize];
        memcpy(d.pData, pDE->pData, d.nSize);
        d.bProcessed = 0;
        d.Name = pDE->Name;
        d.CRC = 0;
        d.nID = snCurListID++;
        Data.insert(ShaderDataItor::value_type(d.Name, d));
        continue;
      }
      if (d.nSize < sizeof(SShaderCacheHeaderItem))
      {
        assert(0);
        continue;
      }
      d.pData = new byte[d.nSize];
      memcpy(d.pData, pDE->pData, d.nSize);
      SShaderCacheHeaderItem *pItem = (SShaderCacheHeaderItem *)d.pData;
      d.bProcessed = 0;
      d.Name = pDE->Name;
      d.CRC = pItem->m_CRC32;
      d.nID = snCurListID++;
      Data.insert(ShaderDataItor::value_type(d.Name, d));
    }
  }

}

struct SNameData
{
  CCryName Name;
  bool bProcessed;
};

void CShaderMan::_MergeShaders()
{
  float t0 = gEnv->pTimer->GetAsyncCurTime();
  SShaderCache *pCache;
  uint32 i, j;

  std::vector<CCryName> NM;
  std::vector<SNameData> Names;
  mfGatherFilesList(m_ShadersMergeCachePath, NM, 0, true);
  for (i=0; i<NM.size(); i++)
  {
    SNameData dt;
    dt.bProcessed = false;
    dt.Name = NM[i];
    Names.push_back(dt);
  }

  uint32 CRC32 = 0;
  for (i=0; i<Names.size(); i++)
  {
    if (Names[i].bProcessed)
      continue;
    Names[i].bProcessed = true;
    const char *szNameA = Names[i].Name.c_str();
    iLog->Log(" Merging shader resource '%s'...", szNameA);
    char szDrv[16], szDir[256], szName[256], szExt[32], szName1[256], szName2[256];
    _splitpath(szNameA, szDrv, szDir, szName, szExt);
    sprintf(szName1, "%s%s", szName, szExt);
    uint32 nLen = strlen(szName1);
    pCache = CHWShader::mfInitCache(szNameA, NULL, false, CRC32, !CRenderer::CV_r_shadersuserfolder, true);
    if (pCache->m_pRes[CACHE_USER])
      CRC32 = pCache->m_Header[CACHE_USER].m_CRC32;
    else
    if (pCache->m_pRes[CACHE_READONLY])
      CRC32 = pCache->m_Header[CACHE_READONLY].m_CRC32;
    else
    {
      assert(0);
    }
    ShaderData Data;
    snCurListID = 0;
    sAddToList(pCache, Data);
    SAFE_RELEASE(pCache);
    for (j=i+1; j<Names.size(); j++)
    {
      if (Names[j].bProcessed)
        continue;
      const char *szNameB = Names[j].Name.c_str();
      _splitpath(szNameB, szDrv, szDir, szName, szExt);
      sprintf(szName2, "%s%s", szName, szExt);
      if (!stricmp(szName1, szName2))
      {
        Names[j].bProcessed = true;
        SShaderCache *pCache1 = CHWShader::mfInitCache(szNameB, NULL, false, 0, !CRenderer::CV_r_shadersuserfolder, true);
        assert(pCache1->m_Header[CACHE_USER].m_CRC32 == CRC32);
        if (pCache1->m_Header[CACHE_USER].m_CRC32 != CRC32)
        {
          Warning("WARNING: CRC mismatch for %s", szNameB);
        }
        sAddToList(pCache1, Data);
        SAFE_RELEASE(pCache1);
      }
    }
    char szDest[256];
    strcpy(szDest, m_ShadersCache);
    const char *p = &szNameA[strlen(szNameA)-nLen-2];
    while (*p != '/' && *p != '\\')
    {
      p--;
    }
    strcat(szDest, p+1);
    pCache = CHWShader::mfInitCache(szDest, NULL, true, CRC32, !CRenderer::CV_r_shadersuserfolder, false);
    CResFile *pRes = pCache->m_pRes[CACHE_USER];
    pRes->mfClose();
    pRes->mfOpen(RA_CREATE);

    float fVersion = (float)FX_CACHE_VER;
    SDirEntry de;
    SShaderCacheHeader hd, hdTemp, *pHD;
    de.Name = s_cNameHEAD;
    de.flags = RF_RES_$HEAD;
    de.size = sizeof(SShaderCacheHeader);
    hd.m_SizeOf = sizeof(SShaderCacheHeader);
    hd.m_MinorVer = (int)(((float)fVersion - (float)(int)fVersion)*10.1f);
    hd.m_MajorVer = (int)fVersion;
    hd.m_bOptimised = false;
    hd.m_CRC32 = CRC32;
    pHD = &hd;
    if (CParserBin::m_bEndians)
    {
      hdTemp = hd;
      SwapEndian(hdTemp, eBigEndian);
      pHD = &hdTemp;
    }
    de.pData = pHD;
    pRes->mfFileAdd(&de);
    pRes->mfFlush(0);
    pCache->m_Header[CACHE_USER] = hd;

    int nDeviceShadersCounter = 0x10000000;
    ShaderDataItor it;
    for (it=Data.begin(); it!=Data.end(); it++)
    {
      SMgData *pD = &it->second;
      SDirEntry de;
      de.Name = pD->Name;
      de.size = pD->nSize;
      de.flags = pD->flags;
      if (pD->flags & RF_RES_$)
        de.flags &= ~RF_COMPRESS;
      else
      {
        de.flags |= RF_COMPRESS;
        de.offset = nDeviceShadersCounter++;
      }
      byte *pNew = new byte[de.size];
      memcpy(pNew, pD->pData, pD->nSize);
      de.pData = pNew;
      de.flags |= RF_TEMPDATA;
      pRes->mfFileAdd(&de);
    }
    for (it=Data.begin(); it!=Data.end(); it++)
    {
      SMgData *pD = &it->second;
      delete [] pD->pData;
    }
    Data.clear();
    pRes->mfFlush(0);
    iLog->Log(" ...%d result items...", pRes->mfGetNumFiles());
    pCache->Release();
  }

  mfOptimiseShaders(gRenDev->m_cEF.m_ShadersCache, true);

  float t1 = gEnv->pTimer->GetAsyncCurTime();
  CryLog("All shaders files merged in %.2f seconds", t1-t0);
}

void CShaderMan::mfMergeShaders()
{
  CHWShader::mfFlushPendedShadersWait(-1);

  CParserBin::SetupForD3D9();
  _MergeShaders();

  CParserBin::SetupForD3D10();
  _MergeShaders();

#if defined (DIRECT3D10)
  CParserBin::SetupForD3D10();
#elif defined (DIRECT3D9)
  CParserBin::SetupForD3D9();
#endif
}



//////////////////////////////////////////////////////////////////////////
bool CShaderMan::CheckAllFilesAreWritable( const char *szDir ) const
{
#if (defined(WIN32) || defined(WIN64))
  assert(szDir);

  ICryPak *pack = gEnv->pCryPak;			assert(pack);

  string sPathWithFilter = string(szDir) + "/*.*";

  // Search files that match filter specification.
  _finddata_t fd;
  int res;
  intptr_t handle;
  if ((handle = pack->FindFirst(sPathWithFilter.c_str(),&fd))!=-1)
  {
    do
    {
      if((fd.attrib & _A_SUBDIR)==0)
      {
        string fullpath = string(szDir) + "/" + fd.name;

        FILE *out = pack->FOpen(fullpath.c_str(),"rb");
        if (!out)
        {
          res = pack->FindNext( handle,&fd );
          continue;
        }
        if (pack->IsInPak(out))
        {
          pack->FClose(out);
          res = pack->FindNext( handle,&fd );
          continue;
        }
        pack->FClose(out);

        out = pack->FOpen(fullpath.c_str(),"ab");

        if(out)
          pack->FClose(out);
        else
        {
          gEnv->pLog->LogError("ERROR: Shader cache is not writable (file: '%s')",fullpath.c_str());
          return false;
        }
      }

      res = pack->FindNext( handle,&fd );
    } while (res >= 0);

    pack->FindClose(handle);

    gEnv->pLog->LogToFile("Shader cache directory '%s' was successfully tested for being writable",szDir);
  }
  else
    CryLog("Shader cache directory '%s' does not exist", szDir);

#endif

  return true;
}
#endif // XENON

