/*=============================================================================
D3DHWShaderCompiling.cpp : D3D specific shaders compiling support.
Copyright (c) 2001-2009 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "IDirectBee.h"
#include <crc32.h>
#if defined(DIRECT3D10) && !defined(PS3)
# include <D3D11Shader.h>
# include <D3DCompiler.h>
#endif
#include "../Common/Shaders/RemoteCompiler.h"

SShaderAsyncInfo SShaderAsyncInfo::m_PendingList;
SShaderAsyncInfo SShaderAsyncInfo::m_PendingListT;
CryEvent SShaderAsyncInfo::m_RequestEv;

int CHWShader_D3D::m_nDevicePSDataSize;
int CHWShader_D3D::m_nDeviceVSDataSize;

class CSpinLock
{
public:
  CSpinLock()
  {
#if defined (WIN32) || defined(XENON)// || defined(PS3)
    while ( CryInterlockedCompareExchange(&s_locked, 1L, 0L) == 1L )
      Sleep(0);
#endif
  }

  ~CSpinLock()
  {
#if defined (WIN32) || defined(XENON)// || defined(PS3)
    InterlockedExchange(&s_locked, 0L);
#endif
  }

private:
  static volatile LONG s_locked;
};

volatile LONG CSpinLock::s_locked = 0L;

volatile int SShaderAsyncInfo::s_nPendingAsyncShaders = 0;
int SShaderAsyncInfo::s_nPendingAsyncShadersFXC = 0;

//==============================================================================================================

void CHWShader_D3D::mfAddFXParameter(SHWSInstance *pInst, SParamsGroup& OutParams, DynArray<SFXParam>& Params, DynArray<STexSampler>& Samplers, SFXParam *pr, const char *ParamName, SCGBind *pBind, CShader *ef, bool bInstParam, EHWShaderClass eSHClass)
{
  SCGParam CGpr;

  assert(pBind);
  if (!pBind)
    return;

  int nComps = 0;
  int nParams = pBind->m_nParameters;
  if (!pr->m_Assign.empty())
    nComps = pr->m_nComps;
  else
  {
    for (int i=0; i<pr->m_nComps; i++)
    {
      string cur = pr->GetParamComp(i);
      if (!cur[0])
        break;
      nComps++;
    }
  }
  // Process parameters only with semantics
  if (nComps && nParams)
  {
    std::vector<SCGParam>* pParams;
    if (pr->m_nParameters > 1)
    {
      if (!bInstParam)
      {
        //if (!pInst->m_pParams[0])
        // pInst->m_pParams[0] = new std::vector<SCGParam>;
        pParams = &OutParams.Params[0];
      }
      else
      {
        //if (!pInst->m_pParams_Inst)
        //  pInst->m_pParams_Inst = new std::vector<SCGParam>;
        pParams = &OutParams.Params_Inst;
      }
    }
    else
      if (bInstParam)
      {
        //if (!pInst->m_pParams_Inst)
        //  pInst->m_pParams_Inst = new std::vector<SCGParam>;
        pParams = &OutParams.Params_Inst;
      }
      else
      {
        //if (!pInst->m_pParams[0])
        //  pInst->m_pParams[0] = new std::vector<SCGParam>;
        pParams = &OutParams.Params[0];
      }
      uint32 nOffs = pParams->size();
      bool bRes = gRenDev->m_cEF.mfParseFXParameter(Params, pr, &Samplers, ParamName, ef, bInstParam, pBind->m_nParameters, pParams, eSHClass, false);
      assert(bRes);
      if (pParams->size() > nOffs)
      {
        for (uint32 i=0; i<pParams->size()-nOffs; i++)
        {
          //assert(pBind->m_nComponents == 1);
          SCGParam &p = (*pParams)[nOffs+i];
          p.m_dwBind = pBind->m_dwBind+i;
#if defined(DIRECT3D10)
          p.m_dwCBufSlot = pBind->m_dwCBufSlot;
#endif
#if defined(OPENGL)
          p.m_isMatrix = pBind->m_isMatrix;
#endif
        }
      }
  }
  // Parameter without semantic
}

struct SAliasSampler
{
  STexSampler *fxSampler;
  string NameINT;
  SAliasSampler()
  {
    fxSampler = NULL;
  }
};

bool CHWShader_D3D::mfAddFXParameter(SHWSInstance *pInst, SParamsGroup& OutParams, DynArray<SFXParam>& Params, DynArray<STexSampler>& Samplers, const char *param, const char *paramINT, SCGBind *bn, bool bInstParam, EHWShaderClass eSHClass, CShader *pFXShader)
{
  SFXParam *pr = gRenDev->m_cEF.mfGetFXParameter(Params, param);
  if (pr)
  {
    if (bn->m_nParameters < 0)
      bn->m_nParameters = pr->m_nParameters;
    mfAddFXParameter(pInst, OutParams, Params, Samplers, pr, paramINT, bn, pFXShader, bInstParam, eSHClass);
    return true;
  }
  return false;
}

//==================================================================================================================

int CGParamCallback( const VOID* arg1, const VOID* arg2 )
{
  SCGParam *pi1 = (SCGParam *)arg1;
  SCGParam *pi2 = (SCGParam *)arg2;
  if (pi1->m_dwBind < pi2->m_dwBind)
    return -1;
  if (pi1->m_dwBind > pi2->m_dwBind)
    return 1;
  return 0;
}

char *szNamesCB[CB_MAX] = {"PER_BATCH", "PER_INSTANCE", "STATIC_INSTANCE", "PER_FRAME", "PER_MATERIAL", "PER_LIGHT", "PER_SHADOWGEN", "SKIN_DATA", "SHAPE_DATA", "INSTANCE_DATA"};
void CHWShader_D3D::mfCreateBinds(SHWSInstance *pInst, LPD3DXCONSTANTTABLE pConstantTable, byte* pShader, int nSize)
{
#if defined (DIRECT3D9) || defined(OPENGL)
  D3DXCONSTANTTABLE_DESC CTDesc;
  pConstantTable->GetDesc(&CTDesc);
  for (uint32 i=0; i<CTDesc.Constants; i++)
  {
    D3DXCONSTANT_DESC CDesc;
    uint32 nCount = 1;
    D3DXHANDLE cHandle = pConstantTable->GetConstant(NULL, i);
    pConstantTable->GetConstantDesc(cHandle, &CDesc, &nCount);
    if (CDesc.RegisterSet == D3DXRS_SAMPLER)
    {
      SCGBind cgp;
      //if (!pInst->m_pBindVars)
      //  pInst->m_pBindVars = new std::vector<SCGBind>;
      cgp.m_dwBind = CDesc.RegisterIndex | SHADER_BIND_SAMPLER;
      cgp.m_Flags = CParserBin::GetCRC32(CDesc.Name);
      cgp.m_nParameters = CDesc.RegisterCount;
      cgp.m_Name = CDesc.Name;
      pInst->m_pBindVars.push_back(cgp);
    }
    else
      if (CDesc.RegisterSet == D3DXRS_FLOAT4 || CDesc.RegisterSet == D3DXRS_INT4 || CDesc.RegisterSet == D3DXRS_BOOL)
      {
        SCGBind cgp;
        cgp.m_dwBind = CDesc.RegisterIndex;
#if defined(OPENGL)
        assert(cgp.m_dwBind < ~scParamMask);//otherwise we have a handle overwriting the bits
        //mark as matrix which important later on where we set the constants
        switch(CDesc.Class)
        {
        case D3DXPC_MATRIX_ROWS2:
          cgp.m_isMatrix = scIs2x4Matrix;
          break;
        case D3DXPC_MATRIX_ROWS3:
          cgp.m_isMatrix = scIs3x4Matrix;
          break;
        case D3DXPC_MATRIX_ROWS4:
          cgp.m_isMatrix = scIs4x4Matrix;
          break;
        }
#endif
        cgp.m_nParameters = CDesc.RegisterCount;
        cgp.m_Name = CDesc.Name;
        cgp.m_Flags = CParserBin::GetCRC32(CDesc.Name);
        //if (!pInst->m_pBindVars)
        //  pInst->m_pBindVars = new std::vector<SCGBind>;
        pInst->m_pBindVars.push_back(cgp);
      }
      else
      {
        assert(false);
      }
  }
#elif defined (DIRECT3D10)
  uint32 i;
  ID3D11ShaderReflection *pShaderReflection = (ID3D11ShaderReflection *)pConstantTable;
  D3D11_SHADER_DESC Desc;
  pShaderReflection->GetDesc(&Desc);
  ID3D11ShaderReflectionConstantBuffer* pCB = NULL;
  for (uint32 n=0; n<Desc.ConstantBuffers; n++)
  {
    pCB = pShaderReflection->GetConstantBufferByIndex(n);
    D3D11_SHADER_BUFFER_DESC SBDesc;
    pCB->GetDesc(&SBDesc);
#if !defined(PS3)
    int nCB;
    if (!strcmp("$Globals", SBDesc.Name))
      nCB = CB_PER_BATCH;
    else
      for (nCB=0; nCB<CB_MAX; nCB++)
      {
        if (!strcmp(szNamesCB[nCB], SBDesc.Name))
          break;
      }
      assert(nCB != CB_MAX);
      if (nCB == CB_MAX)
        continue;
#endif
      for (i=0; i<SBDesc.Variables; i++)
      {
        uint32 nCount = 1;
        ID3D11ShaderReflectionVariable* pCV = pCB->GetVariableByIndex(i);
        ID3D11ShaderReflectionType* pVT = pCV->GetType();
        D3D11_SHADER_VARIABLE_DESC CDesc;
        D3D11_SHADER_TYPE_DESC CTDesc;
        pVT->GetDesc(&CTDesc);
        pCV->GetDesc(&CDesc);
        if (!(CDesc.uFlags & D3D10_SVF_USED))
          continue;
        if (CTDesc.Class==D3D10_SVC_VECTOR || CTDesc.Class==D3D10_SVC_SCALAR || CTDesc.Class==D3D10_SVC_MATRIX_COLUMNS || CTDesc.Class==D3D10_SVC_MATRIX_ROWS)
        {
          SCGBind cgp;
          assert(!(CDesc.StartOffset & 0xf));
          //assert(!(CDesc.Size & 0xf));
          int nReg = CDesc.StartOffset>>4;
          cgp.m_dwBind = nReg; //<<2;



          cgp.m_dwCBufSlot = nCB;

          cgp.m_nParameters = (CDesc.Size+15)>>4;
          cgp.m_Name = CDesc.Name;
          cgp.m_Flags = CParserBin::GetCRC32(CDesc.Name);
          //if (!pInst->m_pBindVars)
          //  pInst->m_pBindVars = new std::vector<SCGBind>;
          pInst->m_pBindVars.push_back(cgp);
        }
        else
        {
          assert(false);
        }
      }
  }
  D3D11_SHADER_INPUT_BIND_DESC IBDesc;
  for (i=0; i<Desc.BoundResources; i++)
  {
    ZeroStruct(IBDesc);
    pShaderReflection->GetResourceBindingDesc(i, &IBDesc);
    if (IBDesc.Type != D3D10_SIT_TEXTURE)
      continue;
    SCGBind cgp;
    //if (!pInst->m_pBindVars)
    // pInst->m_pBindVars = new std::vector<SCGBind>;




    cgp.m_dwBind = IBDesc.BindPoint | SHADER_BIND_SAMPLER;

    cgp.m_nParameters = IBDesc.BindCount;
    cgp.m_Name = IBDesc.Name;
    cgp.m_Flags = CParserBin::GetCRC32(IBDesc.Name);
    pInst->m_pBindVars.push_back(cgp);
  }
#if !defined(PS3)
  if (pInst->m_pBindVars.size())
  {
    for (i=0; i<pInst->m_pBindVars.size(); i++)
    {
      SCGBind *pB = &pInst->m_pBindVars[i];
      if (!(pB->m_dwBind & SHADER_BIND_SAMPLER))
        continue;
      uint32 j;
      for (j=0; j<Desc.BoundResources; j++)
      {
        ZeroStruct(IBDesc);
        pShaderReflection->GetResourceBindingDesc(j, &IBDesc);

        if (IBDesc.Type != D3D10_SIT_SAMPLER )
          continue;

        if (!stricmp(IBDesc.Name, pB->m_Name.c_str()) )
        {
          pB->m_dwCBufSlot = IBDesc.BindPoint;
          break;
        }

        if (!strnicmp(IBDesc.Name, "SAMPLER_STATE_", 14))
        {
          if(strstr(pB->m_Name.c_str(), &(IBDesc.Name[15]))!=NULL)
          {
            pB->m_dwCBufSlot = IBDesc.BindPoint;
            break;
          }
        }

      }
      if (j == Desc.BoundResources /*&& strnicmp(pB->m_Name.c_str(), "sceneDepthSamplerMS", 19)!=0*/)
      {
        //assert(0);
      }
    }
  }
#endif
#endif
}

void CHWShader_D3D::mfGatherFXParameters(SHWSInstance *pInst, std::vector<SCGBind>* BindVars, DynArray<SCGBind> *InstBindVars, CHWShader_D3D *pSH, int nFlags, CShader *pFXShader)
{
  //	LOADING_TIME_PROFILE_SECTION(iSystem);

  uint32 i, j;
  SAliasSampler samps[MAX_TMU];
  int nMaxSampler = -1;
  int nParam = 0;
  SParamsGroup Group;
  if (pInst->m_pBindVars.size())
  {
    for (i=0; i<pInst->m_pBindVars.size(); i++)
    {
      SCGBind *bn = &(*BindVars)[i];
      const char *param = bn->m_Name.c_str();
      if (!strncmp(param, "_g_", 3))
        continue;
      const char *paramINT = param;
      bool bSampler = (bn->m_dwBind & SHADER_BIND_SAMPLER) != 0;
      if (!bSampler)
      {
        if (nFlags < 2)
        {
          bool bRes = mfAddFXParameter(pInst, Group, pSH->m_Params, pSH->m_Samplers, param, paramINT, bn, false, pSH->m_eSHClass, pFXShader);
          if (!bRes)
          {
            iLog->LogWarning("WARNING: Couldn't find parameter '%s' for shader '%s'", param, pSH->GetName());
            // const parameters aren't listed in Params
            // assert(0);
          }
        }
      }
      else
      {
        for (j=0; j<(uint32)pSH->m_Samplers.size(); j++)
        {
          STexSampler *sm = &pSH->m_Samplers[j];
          if (!stricmp(sm->m_Name.c_str(), param))
          {
            int nSampler = bn->m_dwBind & 0xf;
            nMaxSampler = max(nSampler, nMaxSampler);
            samps[nSampler].fxSampler = sm;
#if defined (DIRECT3D10)
            sm->m_nSamplerSlot = (int8)bn->m_dwCBufSlot;
#else
            sm->m_nSamplerSlot = bn->m_dwBind;
#endif
            samps[nSampler].NameINT = paramINT;
            break;
          }
        }
        if (j == pSH->m_Samplers.size())
        {
          for (j=0; j<(uint32)pSH->m_Samplers.size(); j++)
          {
            STexSampler *sm = &pSH->m_Samplers[j];
            const char *src = sm->m_Name.c_str();
            char name[128];
            int n = 0;
            while(src[n])
            {
              if (src[n] <= 0x20 || src[n] == '[')
                break;
              name[n] = src[n];
              n++;
            }
            name[n] = 0;
            if (!stricmp(name, param))
            {
              int nSampler = bn->m_dwBind & 0xf;
#if defined (DIRECT3D10)
              sm->m_nSamplerSlot = (int8)bn->m_dwCBufSlot;
#endif
              for (int nS=0; nS<bn->m_nParameters; nS++)
              {
                nMaxSampler = max(nSampler+nS, nMaxSampler);
                samps[nSampler+nS].fxSampler = sm;
                samps[nSampler+nS].NameINT = paramINT;
              }
              break;
            }
          }
          if (j == pSH->m_Samplers.size())
          {
            assert(0);
          }
        }
      }
    }
  }
  if (nFlags != 1)
  {
    for (i=0; (int)i<=nMaxSampler; i++)
    {
      STexSampler *smp = samps[i].fxSampler;
      if (!smp)
        continue;
      CTexture *tp = gRenDev->m_cEF.mfParseFXTechnique_LoadShaderTexture(smp, NULL, NULL, i, eCO_NOSET, eCO_NOSET, DEF_TEXARG0, DEF_TEXARG0);
      smp->m_pTex = tp;
      //if (!pInst->m_pSamplers)
      //  pInst->m_pSamplers = new DynArray<STexSampler>;
      pInst->m_pSamplers.push_back(*smp);
    }
  }
  else
  {
    assert(pInst->m_pAsync);
    if (pInst->m_pAsync && nMaxSampler >= 0)
      pInst->m_pAsync->m_bPendedSamplers = true;
  }

#if defined (DIRECT3D10)
  pInst->m_nMaxVecs[0] = pInst->m_nMaxVecs[1] = 0;
  if (pInst->m_pBindVars.size())
  {
    for (i=0; i<pInst->m_pBindVars.size(); i++)
    {
      SCGBind *pB = &pInst->m_pBindVars[i];
      if (pB->m_dwBind & SHADER_BIND_SAMPLER)
        continue;
      if (pB->m_dwCBufSlot < 0 || pB->m_dwCBufSlot > 2)
        continue;
      for (j=0; j<Group.Params[0].size(); j++)
      {
        SCGParam *pr = &Group.Params[0][j];
        if (pr->m_dwBind == pB->m_dwBind && pr->m_Name == pB->m_Name)
          break;
      }
      if (j != Group.Params[0].size())
        continue;
      if (pB->m_dwCBufSlot<3)
        pInst->m_nMaxVecs[pB->m_dwCBufSlot] = max(pB->m_dwBind+pB->m_nParameters, pInst->m_nMaxVecs[pB->m_dwCBufSlot]);
    }
  }
#endif
  if (Group.Params[0].size())
  {
    for (i=0; i<Group.Params[0].size(); i++)
    {
      SCGParam *pr = &Group.Params[0][i];

      if (pr->m_Flags & PF_MATERIAL)
        pInst->m_bHasPMParams = true;
    }

#if defined (DIRECT3D10) || defined(PS3)
    if (pSH->m_eSHClass == eHWSC_Vertex)
      pSH->RegisterFX(pFXShader, &Group.Params[0]);
#endif
    gRenDev->m_cEF.mfCheckObjectDependParams(Group.Params[0], Group.Params[1], pSH->m_eSHClass, pFXShader);
  }

#if defined (DIRECT3D10)
  for (i=0; i<2; i++)
  {
    if (Group.Params[i].size())
    {
      for (j=0; j<Group.Params[i].size(); j++)
      {
        SCGParam *pr = &Group.Params[i][j];
        pInst->m_nMaxVecs[i] = max(pr->m_dwBind + pr->m_nParameters, pInst->m_nMaxVecs[i]);
      }
    }
  }
#if !defined(PS3)
  int nMax = 0;
  if (pSH->m_eSHClass == eHWSC_Vertex)
    nMax = MAX_CONSTANTS_VS;
  else
    if (pSH->m_eSHClass == eHWSC_Pixel)
      nMax = MAX_CONSTANTS_PS;
    else
      nMax = MAX_CONSTANTS_GS;


#endif
  assert(pInst->m_nMaxVecs[0] < nMax);
  assert(pInst->m_nMaxVecs[1] < nMax);

#endif

  if ((pInst->m_RTMask & (g_HWSR_MaskBit[HWSR_INSTANCING_ATTR] | g_HWSR_MaskBit[HWSR_INSTANCING_ROT] | g_HWSR_MaskBit[HWSR_INSTANCING_CONST])) && pSH->m_eSHClass == eHWSC_Vertex)
  {
    int nNumInst = 0;
    if (InstBindVars)
    {
      for (i=0; i<(uint32)InstBindVars->size(); i++)
      {
        SCGBind& b = (*InstBindVars)[i];
        int nID = b.m_dwBind;
        if (!nNumInst)
          pInst->m_nInstMatrixID = nID;

        SCGBind bn;
        bn.m_nParameters = b.m_nParameters;
        bn.m_dwBind = nID;
        bool bRes = mfAddFXParameter(pInst, Group, pSH->m_Params, pSH->m_Samplers, b.m_Name.c_str(), b.m_Name.c_str(), &bn, true, pSH->m_eSHClass, pFXShader);

        nNumInst++;
      }
    }
    //assert(cgi->m_nNumInstAttributes == nNumInst);
    pInst->m_nNumInstAttributes = nNumInst;

    if (Group.Params_Inst.size())
    {
      qsort(&Group.Params_Inst[0], Group.Params_Inst.size(), sizeof(SCGParam), CGParamCallback);
      pInst->m_nParams_Inst = CGParamManager::GetParametersGroup(Group.Params_Inst);
    }
  }
  if (Group.Params[0].size() > 0)
  {
    qsort(&Group.Params[0][0], Group.Params[0].size(), sizeof(SCGParam), CGParamCallback);
    pInst->m_nParams[0] = CGParamManager::GetParametersGroup(Group.Params[0]);
  }
  if (Group.Params[1].size() > 0)
  {
    qsort(&Group.Params[1][0], Group.Params[1].size(), sizeof(SCGParam), CGParamCallback);
    pInst->m_nParams[1] = CGParamManager::GetParametersGroup(Group.Params[1]);
  }
}

// Vertex shader specific
void CHWShader_D3D::mfUpdateFXVertexFormat(SHWSInstance *pInst, CShader *pSH)
{
  // Update global FX shader's vertex format / flags
  if (pSH)
  {
    EVertexFormat eVFormat = pSH->m_eVertexFormat;
    bool bCurrent = false;
    for (uint32 i=0; i<pSH->m_HWTechniques.Num(); i++)
    {
      SShaderTechnique *hw = pSH->m_HWTechniques[i];
      for (uint32 j=0; j<hw->m_Passes.Num(); j++)
      {
        SShaderPass *pass = &hw->m_Passes[j];
        if (pass->m_VShader)
        {
          if (pass->m_VShader == this)
            bCurrent = true;
          bool bUseLM = false;
          bool bUseTangs = false;
          bool bUseHWSkin = false;
          bool bUseSH = false;
          EVertexFormat eCurVFormat = pass->m_VShader->mfVertexFormat(bUseTangs, bUseLM, bUseHWSkin, bUseSH);
          if (eCurVFormat >= 0)
            eVFormat = max(eVFormat, eCurVFormat);
          if (bUseTangs)
            pass ->m_PassFlags |= VSM_TANGENTS;
          if (bUseSH)
            pass->m_PassFlags |= VSM_SH;
          if (bUseHWSkin)
          {
            pass->m_PassFlags |= VSM_HWSKIN;
            pass->m_PassFlags |= VSM_HWSKIN_SHAPEDEFORM;
            pass->m_PassFlags |= VSM_HWSKIN_MORPHTARGET;
          }
        }
      }
    }
    assert (bCurrent);
    pSH->m_eVertexFormat = eVFormat;
  }
}

void CHWShader_D3D::mfPostVertexFormat(SHWSInstance *pInst, CHWShader_D3D *pHWSH, bool bCol, byte bNormal, bool bTC0, bool bTC1[2], bool bPSize, bool bTangent[2], bool bBinorm[2], bool bHWSkin, bool bSH[2], bool bShapeDeform, bool bMorphTarget, bool bMorph)
{
  if (bTangent[0] || bBinorm[0])
    pInst->m_VStreamMask_Decl |= 1<<VSF_TANGENTS;
  if (bTangent[1] || bBinorm[1])
    pInst->m_VStreamMask_Stream |= 1<<VSF_TANGENTS;

  if (bHWSkin)
  {
    pInst->m_VStreamMask_Decl |= VSM_HWSKIN;
    pInst->m_VStreamMask_Stream |= VSM_HWSKIN;
  }
  if (bSH[0])
    pInst->m_VStreamMask_Decl |= VSM_SH;
  if (bSH[1])
    pInst->m_VStreamMask_Stream |= VSM_SH;

  if (bShapeDeform)
  {
    pInst->m_VStreamMask_Decl |= VSM_HWSKIN_SHAPEDEFORM;
    pInst->m_VStreamMask_Stream |= VSM_HWSKIN_SHAPEDEFORM;
  }
  if (bMorphTarget)
  {
    pInst->m_VStreamMask_Decl |= VSM_HWSKIN_MORPHTARGET;
    pInst->m_VStreamMask_Stream |= VSM_HWSKIN_MORPHTARGET;
  }
  if (bMorph)
  {
    pInst->m_VStreamMask_Decl |= VSM_MORPHBUDDY;
    pInst->m_VStreamMask_Stream |= VSM_MORPHBUDDY;
  }

  EVertexFormat eVF = VertFormatForComponents(bCol, bTC0, bPSize, bNormal!=0);
  pInst->m_nVertexFormat = eVF;
}

EVertexFormat CHWShader_D3D::mfVertexFormat(bool &bUseTangents, bool &bUseLM, bool &bUseHWSkin, bool& bUseSH)
{
  int i;

  assert (m_eSHClass == eHWSC_Vertex);

  EVertexFormat eVFormat = eVF_P3F_C4B_T2F;
  int nStream = 0;
  for (i=0; i<m_Insts.size(); i++)
  {
    SHWSInstance *pInst = &m_Insts[i];
    eVFormat = (EVertexFormat)max((uint32)eVFormat, (uint32)pInst->m_nVertexFormat);
    nStream |= pInst->m_VStreamMask_Stream;
  }
  bUseTangents = (nStream & VSM_TANGENTS) != 0;
  bUseLM = false;
  bUseHWSkin = (nStream & VSM_HWSKIN) != 0;
  bUseSH = (nStream & VSM_SH) != 0;
  assert (eVFormat < eVF_Max);

  return eVFormat;
}

void CHWShader_D3D::AddMissedInstancedParam(SHWSInstance *pInst, DynArray<SFXParam>& Params, int nIndex, DynArray<SCGBind>& InstBindVars)
{
  int i;
  for (i=0; i<InstBindVars.size(); i++)
  {
    SCGBind &b = InstBindVars[i];
    if (b.m_dwBind == nIndex)
    {
      SFXParam *pr = gRenDev->m_cEF.mfGetFXParameter(Params, b.m_Name.c_str());
      if (!pr)
      {
        //if (!pInst->m_pParams_Inst)
        //  pInst->m_pParams_Inst = new std::vector<SCGParam>;
        SCGParam param;
        param.m_dwBind = b.m_dwBind;
        param.m_Flags = b.m_Flags | PF_SINGLE_COMP;
        param.m_Name = b.m_Name;
        param.m_nParameters = b.m_nParameters;
        param.m_eCGParamType = ECGP_Unknown;
        //pInst->m_pParams_Inst.push_back(param);
        break;
      }
    }
  }
}

#if !defined(PS3)

void CHWShader_D3D::AnalyzeSemantic(SHWSInstance *pInst, DynArray<SFXParam>& Params, D3DXSEMANTIC *pSM, bool bUsed, bool& bPos, byte& bNormal, bool bTangent[2], bool bBinorm[2], bool& bHWSkin, bool& bShapeDeform, bool& bMorphTarget, bool& bBoneSpace, bool& bPSize, bool bSH[], bool& bMorph, bool& bTC0, bool bTC1[], bool& bCol, bool& bSecCol, DynArray<SCGBind>& InstBindVars)
{
  switch (pSM->Usage)
  {
  case D3DDECLUSAGE_POSITION:
    if (pSM->UsageIndex == 0)
      bPos = true;
    //#ifndef PS3
    else
    if (pSM->UsageIndex == 3)
      bMorphTarget = true;
    else
    if (pSM->UsageIndex == 4)
      bHWSkin = true;
    else
    if (pSM->UsageIndex == 8)
      bMorph = true;
//#endif







    else
      assert(false);
    break;

  case D3DDECLUSAGE_NORMAL:
    bNormal = true;
    break;

  case D3DDECLUSAGE_TEXCOORD:
    if (pSM->UsageIndex == 0)
      bTC0 = true;
    else
    if (pSM->UsageIndex > 0 && (pInst->m_RTMask & g_HWSR_MaskBit[HWSR_INSTANCING_ATTR]))
    {
      AddMissedInstancedParam(pInst, Params, pSM->UsageIndex, InstBindVars);
    }
    else
    if (pSM->UsageIndex == 1)
    {
      bTC1[0] = true;
      bTC1[1] = bUsed;
    }
    else
    if (pSM->UsageIndex >= 4 && pSM->UsageIndex <= 5)
    {
      bShapeDeform = true;
    }
    else
    if (pSM->UsageIndex == 8)
      bMorph = true;
    break;

  case D3DDECLUSAGE_COLOR:
    if (pSM->UsageIndex == 0)
      bCol = true;
    else
    if (pSM->UsageIndex == 1)
    {
      //assert(0);
      bSecCol = true;
    }
    else
    if (pSM->UsageIndex == 2 || pSM->UsageIndex == 3)
    {
      bSH[0] = true;
      bSH[1] = bUsed;
    }
    else
      assert(false);
    break;

    //#ifndef PS3
  case D3DDECLUSAGE_TANGENT:
    bTangent[0] = true;
    bTangent[1] = bUsed;
    break;
  case D3DDECLUSAGE_BINORMAL:
    bBinorm[0] = true;
    bBinorm[1] = bUsed;
    break;

  case D3DDECLUSAGE_PSIZE:
    bPSize = true;
    break;

  case D3DDECLUSAGE_BLENDWEIGHT:
  case D3DDECLUSAGE_BLENDINDICES:
    if (pSM->UsageIndex == 0)
      bHWSkin = true;
    else
    if (pSM->UsageIndex == 1)
      bMorph = true;
    else
      assert(0);
    break;
  default:
    {
      assert(0);
    }
  }
}

bool sCreateSemantic(D3DXSEMANTIC& SM, bool& bUsed, char *sName, char *sIndex, char *sMask, char *sReg, char *sSys, char *sFormat, char *sUsed)
{
  bUsed = true;
  if (!sUsed[0])
    bUsed = false;

  memset(&SM, 0, sizeof(D3DXSEMANTIC));
  if (!strcmp(sName, "POSITION"))
    SM.Usage = D3DDECLUSAGE_POSITION;
  else
  if (!strcmp(sName, "TEXCOORD"))
    SM.Usage = D3DDECLUSAGE_TEXCOORD;
  else
  if (!strcmp(sName, "COLOR"))
    SM.Usage = D3DDECLUSAGE_COLOR;
  else
  if (!strcmp(sName, "TANGENT"))
    SM.Usage = D3DDECLUSAGE_TANGENT;
  else
  if (!strcmp(sName, "BINORMAL"))
    SM.Usage = D3DDECLUSAGE_BINORMAL;
  else
  if (!strcmp(sName, "PSIZE"))
    SM.Usage = D3DDECLUSAGE_PSIZE;
  else
  if (!strcmp(sName, "BLENDWEIGHT"))
    SM.Usage = D3DDECLUSAGE_BLENDWEIGHT;
  else
  if (!strcmp(sName, "BLENDINDICES"))
    SM.Usage = D3DDECLUSAGE_BLENDINDICES;
  else
  if (!strcmp(sName, "NORMAL"))
    SM.Usage = D3DDECLUSAGE_NORMAL;
  else
  {
    assert(0);
    return false;
  }
  SM.UsageIndex = shGetInt(sIndex);

  return true;
}
#endif


static bool sGetStr(char *& sS, char *szDst)
{
  int n = 0;
  szDst[n] = 0;
  while(sS[0]==0x20 || sS[0]==8) { sS++; }
  if (sS[0]=='\n')
    return false;
  if (!sS[0])
    return false;
  shFill(&sS, szDst, 32);
  SkipCharacters(&sS, kWhiteSpace);
  return true;
}

EVertexFormat CHWShader_D3D::mfVertexFormat(SHWSInstance *pInst, CHWShader_D3D *pSH, LPD3DXBUFFER pShader, DynArray<SCGBind>& InstBindVars)
{
  /*if (!stricmp(pSH->m_EntryFunc.c_str(), "ParticleVS"))
  {
    int nnn = 0;
  }*/

  assert (pSH->m_eSHClass == eHWSC_Vertex);

  byte bNormal = false;
  bool bTangent[2] = {false, false};
  bool bBinorm[2] = {false, false};
  bool bHWSkin = false;
  bool bShapeDeform = false;
  bool bMorphTarget = false;
  bool bMorph = false;
  bool bBoneSpace = false;
  bool bPSize = false;
  bool bSH[2] = {false, false};
  bool bTC0 = false;
  bool bTC1[2] = {false, false};
  bool bCol = false;
  bool bSecCol = false;
  bool bPos = false;
  EVertexFormat eVFormat = eVF_P3F_C4B_T2F;

#if defined (DIRECT3D9) || defined (OPENGL)

  D3DXSEMANTIC Semantics[MAXD3DDECLLENGTH];
  uint32 nCounts;

  if (!CParserBin::m_bD3D11)
  {
    HRESULT hr = D3DXGetShaderInputSemantics((DWORD *)pShader->GetBufferPointer(), Semantics, &nCounts);
    assert(SUCCEEDED(hr));
    if (!FAILED(hr))
    {
      for (uint32 i=0; i<nCounts; i++)
      {
        D3DXSEMANTIC *pSM = &Semantics[i];
        if (pSM->UsageIndex == (unsigned)-1)
          continue;
        AnalyzeSemantic(pInst, pSH->m_Params, pSM, true, bPos, bNormal, bTangent, bBinorm, bHWSkin, bShapeDeform, bMorphTarget, bBoneSpace, bPSize, bSH, bMorph, bTC0, bTC1, bCol, bSecCol, InstBindVars);
      }
    }
    mfPostVertexFormat(pInst, pSH, bCol, bNormal, bTC0, bTC1, bPSize, bTangent, bBinorm, bHWSkin, bSH, bShapeDeform, bMorphTarget, bMorph);
  }
  else
  {
    /*if (CParserBin::m_bPS3)
    {
    D3DX11Dis
    }*/
    char *pB = (char *)pShader->GetBufferPointer();
    char *sS = strstr(pB, "Input signature:");
    assert(sS);
    sS = strstr(sS, "-----");
    assert(sS);
    while(*sS != '\n') {sS++;}
    sS++;
    while (true)
    {
      char sSrc[256], sC[8], sName[32], sIndex[8], sMask[8], sReg[8], sSys[8], sFormat[8], sUsed[8];
      char *pS = sSrc;
      fxFillCR(&sS, sSrc);
      sGetStr(pS, sC);
      if (!sGetStr(pS, sName))
        break;
      sGetStr(pS, sIndex);
      sGetStr(pS, sMask);
      sGetStr(pS, sReg);
      sGetStr(pS, sSys);
      sGetStr(pS, sFormat);
      sGetStr(pS, sUsed);
      assert(sC[0] == '/');
      D3DXSEMANTIC SM;
      bool bUsed;
      bool bRes = sCreateSemantic(SM, bUsed, sName, sIndex, sMask, sReg, sSys, sFormat, sUsed);
      if (bRes)
        AnalyzeSemantic(pInst, pSH->m_Params, &SM, bUsed, bPos, bNormal, bTangent, bBinorm, bHWSkin, bShapeDeform, bMorphTarget, bBoneSpace, bPSize, bSH, bMorph, bTC0, bTC1, bCol, bSecCol, InstBindVars);
    }
    mfPostVertexFormat(pInst, pSH, bCol, bNormal, bTC0, bTC1, bPSize, bTangent, bBinorm, bHWSkin, bSH, bShapeDeform, bMorphTarget, bMorph);
  }

#elif defined (DIRECT3D10) || defined (PS3)
  ID3D11ShaderReflection *pShaderReflection;
  UINT nSize = pShader->GetBufferSize();
  void *pData = pShader->GetBufferPointer();
  HRESULT hr = D3DReflect(pData, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
  assert (SUCCEEDED(hr));
  if (!SUCCEEDED(hr))
    return eVF_Unknown;
  D3D11_SHADER_DESC Desc;
  pShaderReflection->GetDesc(&Desc);
  if (!Desc.InputParameters)
    return eVF_Unknown;
  D3D11_SIGNATURE_PARAMETER_DESC IDesc;
  for (uint32 i=0; i<Desc.InputParameters; i++)
  {
    pShaderReflection->GetInputParameterDesc(i, &IDesc);
    //if (!IDesc.ReadWriteMask)
    //  continue;
    if (!IDesc.SemanticName)
      continue;
    int nIndex;
    if (!strnicmp(IDesc.SemanticName, "POSITION", 8) || !strnicmp(IDesc.SemanticName, "SV_POSITION", 11))
    {
      nIndex = IDesc.SemanticIndex;
      if (nIndex == 0)
        bPos = true;
      else
      if (nIndex == 3)
        bMorphTarget = true;
      else
      if (nIndex == 4)
        bHWSkin = true;
      else
      if (nIndex == 8)
        bMorph = true;
      else
        assert(false);
    }
    else
    if (!strnicmp(IDesc.SemanticName, "NORMAL", 6))
    {
      bNormal = true;
    }
    else
    if (!strnicmp(IDesc.SemanticName, "TEXCOORD", 8))
    {
      nIndex = IDesc.SemanticIndex;
      if (nIndex == 0)
        bTC0 = true;
      else
      {
        if (nIndex > 0 && (pInst->m_RTMask & g_HWSR_MaskBit[HWSR_INSTANCING_ATTR]))
        {
          AddMissedInstancedParam(pInst, pSH->m_Params, nIndex, InstBindVars);
        }
        else
        if (nIndex == 1)
        {
          bTC1[0] = true;
          if (IDesc.ReadWriteMask)
            bTC1[1] = true;
        }
        else
        if (nIndex >= 4 && nIndex <= 5)
        {
          bShapeDeform = true;
        }
        else
        if (nIndex == 8)
          bMorph = true;
      }
    }
    else
    if (!strnicmp(IDesc.SemanticName, "COLOR", 5))
    {
      nIndex = IDesc.SemanticIndex;
      if (nIndex == 0)
        bCol = true;
      else
      if (nIndex == 1)
        bSecCol = true;
      else
      {
        if (nIndex == 2 || nIndex == 3)
        {
          bSH[0] = true;
          if (IDesc.ReadWriteMask)
            bSH[1] = true;
        }
        else
          assert(false);
      }
    }
    else
    if (!stricmp(IDesc.SemanticName, "TANGENT"))
    {
      bTangent[0] = true;
      if (IDesc.ReadWriteMask)
        bTangent[1] = true;
    }
    else
    if (!stricmp(IDesc.SemanticName, "BINORMAL"))
    {
      bBinorm[0] = true;
      if (IDesc.ReadWriteMask)
        bBinorm[1] = true;
    }
    else
    if (!strnicmp(IDesc.SemanticName, "PSIZE", 5))
    {
      bPSize = true;
    }
    else
    if (!strnicmp(IDesc.SemanticName, "BLENDWEIGHT", 11) || !strnicmp(IDesc.SemanticName, "BLENDINDICES", 12))
    {
      nIndex = IDesc.SemanticIndex;
      if (nIndex == 0)
        bHWSkin = true;
      else
      if (nIndex == 1)
        bMorph = true;
      else
        assert(0);
    }
    else
    {
      assert(0);
    }
  }
  mfPostVertexFormat(pInst, pSH, bCol, bNormal, bTC0, bTC1, bPSize, bTangent, bBinorm, bHWSkin, bSH, bShapeDeform, bMorphTarget, bMorph);
  SAFE_RELEASE(pShaderReflection);
#endif
  return (EVertexFormat)pInst->m_nVertexFormat;
}

void CHWShader_D3D::mfSetDefaultRT(uint64& nAndMask, uint64& nOrMask)
{
  uint32 i, j;
  SShaderGen *pGen = gRenDev->m_cEF.m_pGlobalExt;

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

  // Make a mask of flags affected by this type of shader
  uint32 nType = m_dwShaderType;
  if (nType)
  {
    for (i=0; i<pGen->m_BitMask.size(); i++)
    {
      SShaderGenBit *pBit = pGen->m_BitMask[i];
      if (!pBit->m_Mask)
        continue;
      if (nBitsPlatform & pBit->m_nDependencyReset)
      {
        nAndMask &= ~pBit->m_Mask;
        continue;
      }
      for (j=0; j<pBit->m_PrecacheNames.size(); j++)
      {
        if (pBit->m_PrecacheNames[j] == nType)
        {
          if (nBitsPlatform & pBit->m_nDependencySet)
            nOrMask |= pBit->m_Mask;
          break;
        }
      }
    }
  }
}

//==================================================================================================================

static TArray<char> sNewScr;

static bool sGetMask(char *str, SShaderGen *pGen, uint64& nMask)
{
  uint32 i;

  for (i=0; i<pGen->m_BitMask.Num(); i++)
  {
    SShaderGenBit *pBit = pGen->m_BitMask[i];
    if (!strcmp(str, pBit->m_ParamName.c_str()))
    {
      nMask |= pBit->m_Mask;
      return true;
    }
  }
  return false;
}

bool CHWShader_D3D::mfStoreCacheTokenMap(FXShaderToken*& Table, DynArray<uint32>*& pSHData, const char *szName)
{
  TArray<byte> Data;

  FXShaderTokenItor itor;
  uint32 nSize = pSHData->size();
  if (CParserBin::m_bEndians)
  {
    uint32 nSizeEnd = nSize;
    SwapEndian(nSizeEnd, eBigEndian);
    Data.Copy((byte *)&nSizeEnd, sizeof(uint32));
    for (uint32 i=0; i<nSize; i++)
    {
      uint32 nToken = (*pSHData)[i];
      SwapEndian(nToken, eBigEndian);
      Data.Copy((byte *)&nToken, sizeof(uint32));
    }
  }
  else
  {
    Data.Copy((byte *)&nSize, sizeof(uint32));
    Data.Copy((byte *)&(*pSHData)[0], nSize*sizeof(uint32));
  }
  for (itor=Table->begin(); itor!=Table->end(); itor++)
  {
    uint32 nTok = itor->first;
    if (CParserBin::m_bEndians)
      SwapEndian(nTok, eBigEndian);
    Data.Copy((byte *)&nTok, sizeof(DWORD));
    Data.Copy((byte *)itor->second.SToken.c_str(), itor->second.SToken.size()+1);
  }
  if (!Data.size())
    return false;
  SDirEntry de;
  de.Name = szName;
  de.flags = RF_RES_$MAP;
  de.size = Data.size();
  de.pData = &Data[0];
  m_pCache->m_pRes[CACHE_USER]->mfFileAdd(&de);
  m_pCache->m_pRes[CACHE_USER]->mfFlush();

  return true;
}

void CHWShader_D3D::mfGetTokenMap(CResFile *pRes, SDirEntry *pDE, FXShaderToken*& Table, DynArray<uint32>*& pSHData)
{
  int nSize = pRes->mfFileRead(pDE);
  byte *pData = (byte *)pRes->mfFileGetBuf(pDE);
  Table = new FXShaderToken;
  pSHData = new DynArray<uint32>;
  uint32 nL = *(uint32 *)pData;
  if (CParserBin::m_bEndians)
    SwapEndian(nL, eBigEndian);
  pSHData->resize(nL);
  if (CParserBin::m_bEndians)
  {
    uint32 *pTokens = (uint32 *)&pData[4];
    for (uint32 i=0; i<nL; i++)
    {
      uint32 nToken = pTokens[i];
      SwapEndian(nToken, eBigEndian);
      (*pSHData)[i] = nToken;
    }
  }
  else
  {
    memcpy(&(*pSHData)[0], &pData[4], nL*sizeof(uint32));
  }
  pData += 4 + nL*sizeof(uint32);
  nSize -= 4 + nL*sizeof(uint32);
  int nOffs = 0;
  while (nOffs < nSize)
  {
    char *pStr = (char *)&pData[nOffs+sizeof(DWORD)];
    DWORD nToken = *(DWORD *)&pData[nOffs];
    if (CParserBin::m_bEndians)
      SwapEndian(nToken, eBigEndian);
    int nLen = strlen(pStr)+1;
    STokenD TD;
    TD.SToken = pStr;
    Table->insert(FXShaderTokenItor::value_type(nToken, TD));
    nOffs += sizeof(DWORD) + nLen;
  }
}

bool CHWShader_D3D::mfGetCacheTokenMap(FXShaderToken*& Table, DynArray<uint32>*& pSHData, uint64 nMaskGenFX)
{
  if (!m_pCache || !m_pCache->m_pRes[CACHE_READONLY] || m_pCache->m_bPlatformD3D11 != CParserBin::m_bD3D11 || m_pCache->m_bPlatformXenon != CParserBin::m_bXenon || m_pCache->m_bPlatformPS3 != CParserBin::m_bPS3)
  {
    if (m_pCache)
      m_pCache->m_nRefCount--;
    m_pCache = mfInitCache(NULL, this, true, m_CRC32, !CRenderer::CV_r_shadersuserfolder, true);
  }
  if (!m_pCache)
  {
    assert(false);
    return false;
  }

  char strName[256];
  if (m_Flags & HWSG_SHARED)
  {
    char nm[256];
    nm[0] = 0;
    _splitpath(m_NameSourceFX.c_str(), NULL, NULL, nm, NULL);



    sprintf(strName, "$MAP_%I64x_%s", nMaskGenFX, nm);

  }
  else



    sprintf(strName, "$MAP_%I64x", nMaskGenFX);

  if (Table)
  {
    if (m_pCache->m_pRes[CACHE_READONLY] && m_pCache->m_pRes[CACHE_READONLY]->mfFileExist(strName))
      return true;
    if (!m_pCache->m_pRes[CACHE_USER])
    {
      m_pCache->m_nRefCount--;
      m_pCache = mfInitCache(NULL, this, true, m_CRC32, false, false);
    }
    if (!m_pCache || !m_pCache->m_pRes[CACHE_USER])
    {
      assert(false);
      return false;
    }
    if (!m_pCache->m_pRes[CACHE_USER]->mfFileExist(strName))
    {
      if (CRenderer::CV_r_shadersnocompile)
        return false;
      return mfStoreCacheTokenMap(Table, pSHData, strName);
    }
    return true;
  }
  SDirEntry *pDE = NULL;
  CResFile *pRes = NULL;
  for (int i=0; i<2; i++)
  {
    pRes = m_pCache->m_pRes[i];
    if (!pRes)
      continue;
    pDE = pRes->mfGetEntry(strName);
    if (pDE)
      break;
  }
  if (!pDE || !pRes)
  {
    Warning("Couldn't find tokens MAP entry '%s' in shader cache file '%s'", strName, m_pCache->m_Name.c_str());
    assert(0);
    return false;
  }
  mfGetTokenMap(pRes, pDE, Table, pSHData);
  pRes->mfFileClose(pDE);

  return true;
}

#if !defined (XENON) && !defined(PS3)
bool CParserBin::mfModifyFlags_r(SPreprocessTree *pTree, SPreprocessMasks& Masks, std::vector<SPreprocessNode *>& Nodes)
{
  CRenderer *pRD = gRenDev;
  for (int i=0; i<Nodes.size(); i++)
  {
    SPreprocessNode *pNode = Nodes[i];
    if (pNode->m_RTMask)
      Masks.nRT |= (pNode->m_RTMask & Masks.nRTSet);
    if (pNode->m_GLMask)
      Masks.nGL |= (pNode->m_GLMask & Masks.nGLSet);
    if (pNode->m_LTMask)
      Masks.nLT |= (pNode->m_LTMask & Masks.nLTSet);
    int nNode = IsPreprocessExprTrue(pTree, pNode->m_Expression, Masks) ? 0 : 1;
    if (pNode->m_Nodes[nNode].size())
      mfModifyFlags_r(pTree, Masks, pNode->m_Nodes[nNode]);
  }

  return true;
}
#endif

bool CHWShader_D3D::mfModifyFlags(CShader *pSH)
{
#if !defined (XENON) && !defined(PS3)
  SPreprocessTree *pTree = m_pTree;
  CRenderer *pRD = gRenDev;
  CParserBin Parser(NULL, pSH);

  bool bRes = true;
  if (!pTree || pTree->m_pSH != pSH)
  {
    SAFE_DELETE(pTree);

    m_pTree = NULL;
    FXShaderToken* Table = NULL;
    DynArray<uint32>* pSHData = NULL;
    bRes = mfGetCacheTokenMap(Table, pSHData, m_nMaskGenShader);
    if (!bRes)
    {
      SAFE_DELETE(Table);
      SAFE_DELETE(pSHData);
      return false;
    }
    pTree = Parser.BuildPreprocessTree(pSHData, Table, pSH);
    SAFE_DELETE(Table);
    SAFE_DELETE(pSHData);
    if (!pTree)
      return false;
    pTree->m_pSH = pSH;
    m_pTree = pTree;
  }
  SPreprocessMasks Masks;
  ModifyLTMask(pRD->m_RP.m_FlagsShader_LT);
  Masks.nRT = 0; Masks.nRTSet = pRD->m_RP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
  Masks.nGL = 0; Masks.nGLSet = m_nMaskGenShader;
  Masks.nLT = 0; Masks.nLTSet = pRD->m_RP.m_FlagsShader_LT;
  Masks.nMD = 0; Masks.nMDSet = pRD->m_RP.m_FlagsShader_MD;
  Masks.nMDV = 0; Masks.nMDVSet = pRD->m_RP.m_FlagsShader_MDV;

  if (Masks.nRTSet == 0x70000000)
  {
    int nnn = 0;
  }

  bRes = Parser.mfModifyFlags_r(pTree, Masks, pTree->m_Root);

  pRD->m_RP.m_FlagsShader_RT = Masks.nRT;
  //m_nMaskGenShader = Masks.nGL;
  if (!Masks.nLT)
    pRD->m_RP.m_FlagsShader_LT = 0;

  return bRes;


#endif
}

//==============================================================================================================================================================

char *CHWShader_D3D::mfGenerateScript(SHWSInstance *&pInst, DynArray<SCGBind>& InstBindVars, uint32 nFlags, FXShaderToken* Table, DynArray<uint32>* pSHData)
{
  char *cgs = NULL;

  sNewScr.SetUse(0);
  bool bTempMap = (Table == NULL);
  assert((Table && pSHData) || (!Table && !pSHData));
  assert (m_pCache);
  if (m_pCache)
    mfGetCacheTokenMap(Table, pSHData, m_nMaskGenShader);
  if (CParserBin::m_bEditable)
  {
    if (bTempMap)
    {
      SAFE_DELETE(Table);
      SAFE_DELETE(pSHData);
    }
    Table = &m_Table;
    pSHData = &m_TokenData;
    bTempMap = false;
  }
  assert (Table && pSHData);
  if (!Table || !pSHData)
    return NULL;

  DynArray<uint32> NewTokens;

  uint32 eT = eT_unknown;

  switch (pInst->m_eClass)
  {
  case eHWSC_Vertex:
    eT = eT__VS;
    break;
  case eHWSC_Pixel:
    eT = eT__PS;
    break;
  case eHWSC_Geometry:
    eT = eT__GS;
    break;

  default:
    assert(0);
  }
  if (eT != eT_unknown)
    CParserBin::AddDefineToken(eT, NewTokens);

  // Include runtime mask definitions in the script
  SShaderGen *shg = gRenDev->m_cEF.m_pGlobalExt;
  if (shg && pInst->m_RTMask)
  {
    for (uint32 i=0; i<shg->m_BitMask.Num(); i++)
    {
      SShaderGenBit *bit = shg->m_BitMask[i];
      if (!(bit->m_Mask & pInst->m_RTMask))
        continue;
      CParserBin::AddDefineToken(bit->m_dwToken, NewTokens);
    } 
  }

  // Include light mask definitions in the script
  if (m_Flags & HWSG_SUPPORTS_MULTILIGHTS)
  {
    int nLights = pInst->m_LightMask & 0xf;
    if (nLights)
      CParserBin::AddDefineToken(eT__LT_LIGHTS, NewTokens);
    CParserBin::AddDefineToken(eT__LT_NUM, nLights+eT_0, NewTokens);
    bool bHasProj = false;
    for (int i=0; i<4; i++)
    {
      int nLightType = (pInst->m_LightMask >> (SLMF_LTYPE_SHIFT + i*SLMF_LTYPE_BITS)) & SLMF_TYPE_MASK;
      if (nLightType == SLMF_PROJECTED)
        bHasProj = true;

      CParserBin::AddDefineToken(eT__LT_0_TYPE+i, nLightType+eT_0, NewTokens);
    }
    if (bHasProj)
      CParserBin::AddDefineToken(eT__LT_HASPROJ, eT_1, NewTokens);
  }
  else
  if (m_Flags & HWSG_SUPPORTS_LIGHTING)
  {
    CParserBin::AddDefineToken(eT__LT_LIGHTS, NewTokens);
    int nLightType = (pInst->m_LightMask >> SLMF_LTYPE_SHIFT) & SLMF_TYPE_MASK;
    if (nLightType == SLMF_PROJECTED)
      CParserBin::AddDefineToken(eT__LT_HASPROJ, eT_1, NewTokens);
  }

  // Include modificator mask definitions in the script
  if ((m_Flags & HWSG_SUPPORTS_MODIF) && pInst->m_MDMask)
  {
    for (int nt=0; nt<4; nt++)
    {
      uint32 tcGOLMask = HWMD_TCGOL0<<nt;
      uint32 tcGRMMask = HWMD_TCGRM0<<nt;
      uint32 tcGNMMask = HWMD_TCGNM0<<nt;
      uint32 tcGSMMask = HWMD_TCGSM0<<nt;
      uint32 tcMMask   = HWMD_TCM0<<nt;
      uint32 tcProjMask = HWMD_TCPROJ0<<nt;
      uint32 tcTypeMask = HWMD_TCTYPE0<<nt;
      if (pInst->m_MDMask & tcTypeMask)
        CParserBin::AddDefineToken(eT__TT0_TCUBE+nt, NewTokens);
      if (pInst->m_MDMask & tcProjMask)
        CParserBin::AddDefineToken(eT__TT0_TCPROJ+nt, NewTokens);
      if (pInst->m_MDMask & tcMMask)
        CParserBin::AddDefineToken(eT__TT0_TCM+nt, NewTokens);
      if (pInst->m_MDMask & (tcGOLMask | tcGRMMask | tcGNMMask | tcGSMMask))
      {
        int nType = 0;
        if (pInst->m_MDMask & tcGOLMask)
          nType = 1;
        else
        if (pInst->m_MDMask & tcGRMMask)
          nType = 2;
        else
        if (pInst->m_MDMask & tcGNMMask)
          nType = 3;
        else
        if (pInst->m_MDMask & tcGSMMask)
          nType = 4;
        CParserBin::AddDefineToken(eT__TT0_TCG_TYPE, eT_0+nType, NewTokens);
      }
    }
  }

  // Include vertex modificator mask definitions in the script
  if ((m_Flags & HWSG_SUPPORTS_VMODIF) && pInst->m_MDVMask)
  {
    int nType = pInst->m_MDVMask & 0xf;
    if (nType)
      CParserBin::AddDefineToken(eT__VT_TYPE, eT_0+nType, NewTokens);
    if ((pInst->m_MDVMask & MDV_BENDING) || nType == eDT_Bending)
    {
      CParserBin::AddDefineToken(eT__VT_BEND, eT_1, NewTokens);
      if (!(pInst->m_MDVMask & 0xf))
      {
        nType = eDT_Bending;
        CParserBin::AddDefineToken(eT__VT_TYPE, eT_0+nType, NewTokens);
      }
    }
    if (pInst->m_MDVMask & MDV_DEPTH_OFFSET)
      CParserBin::AddDefineToken(eT__VT_DEPTH_OFFSET, eT_1, NewTokens);
    if (pInst->m_MDVMask & MDV_WIND)
      CParserBin::AddDefineToken(eT__VT_WIND, eT_1, NewTokens);
    if (pInst->m_MDVMask & MDV_DET_BENDING)
      CParserBin::AddDefineToken(eT__VT_DET_BEND, eT_1, NewTokens);
    if (pInst->m_MDVMask & MDV_DET_BENDING_GRASS)
      CParserBin::AddDefineToken(eT__VT_GRASS, eT_1, NewTokens);
    if (pInst->m_MDVMask & MDV_TERRAIN_ADAPT)
      CParserBin::AddDefineToken(eT__VT_TERRAIN_ADAPT, eT_1, NewTokens);
    if (pInst->m_MDVMask & ~0xf)
      CParserBin::AddDefineToken(eT__VT_TYPE_MODIF, eT_1, NewTokens);
  }

  if (m_Flags & HWSG_FP_EMULATION)
  {
    CParserBin::AddDefineToken(eT__FT0_COP, eT_0+(pInst->m_LightMask&0xff), NewTokens);
    CParserBin::AddDefineToken(eT__FT0_AOP, eT_0+((pInst->m_LightMask&0xff00)>>8), NewTokens);

    byte CO_0 = ((pInst->m_LightMask&0xff0000) >> 16) & 7;
    CParserBin::AddDefineToken(eT__FT0_CARG1, eT_0+CO_0, NewTokens);

    byte CO_1 = ((pInst->m_LightMask&0xff0000) >> 19) & 7;
    CParserBin::AddDefineToken(eT__FT0_CARG2, eT_0+CO_1, NewTokens);

    byte AO_0 = ((pInst->m_LightMask&0xff000000) >> 24) & 7;
    CParserBin::AddDefineToken(eT__FT0_AARG1, eT_0+AO_0, NewTokens);

    byte AO_1 = ((pInst->m_LightMask&0xff000000) >> 27) & 7;
    CParserBin::AddDefineToken(eT__FT0_AARG2, eT_0+AO_1, NewTokens);

    if (CO_0 == eCA_Specular || CO_1 == eCA_Specular || AO_0 == eCA_Specular || AO_1 == eCA_Specular)
      CParserBin::AddDefineToken(eT__FT_SPECULAR, NewTokens);
    if (CO_0 == eCA_Diffuse || CO_1 == eCA_Diffuse || AO_0 == eCA_Diffuse || AO_1 == eCA_Diffuse)
      CParserBin::AddDefineToken(eT__FT_DIFFUSE, NewTokens);
    if (CO_0 == eCA_Texture || CO_1 == eCA_Texture || AO_0 == eCA_Texture || AO_1 == eCA_Texture)
      CParserBin::AddDefineToken(eT__FT_TEXTURE, NewTokens);
  }

  int nT = NewTokens.size();
  NewTokens.resize(nT + pSHData->size());
  memcpy(&NewTokens[nT], &(*pSHData)[0], pSHData->size()*sizeof(uint32));

  CParserBin Parser(NULL, gRenDev->m_RP.m_pShader);
  Parser.Preprocess(1, NewTokens, Table);
  CorrectScriptEnums(Parser, pInst, InstBindVars, Table);
  RemoveUnaffectedParameters_D3D10(Parser, pInst, InstBindVars);
  ConvertBinScriptToASCII(Parser, pInst, InstBindVars, Table, sNewScr);

  if (bTempMap)
  {
    SAFE_DELETE(Table);
    SAFE_DELETE(pSHData);
  }

 /* FILE *fp = gEnv->pCryPak->FOpen("fff", "w");
  if (fp)
  {
    gEnv->pCryPak->FPrintf(fp, "%s", &sNewScr[0]);
    gEnv->pCryPak->FClose (fp);
  }*/

  return &sNewScr[0];
}

/*static uint32 sFindVar(CParserBin& Parser, int& nStart)
{
  const uint32 *pTokens = Parser.GetTokens(0);
  int nLast = Parser.GetNumTokens()-1;

  while (nStart <= nLast)
  {
    if (pTokens[nStart] == eT_br_cv_1)
    {
      int nRecurs = 1;
      nStart++;
      while(nStart <= nLast)
      {
        if (pTokens[nStart++] == eT_br_cv_1)
          nRecurs++;
        else
        if (pTokens[nStart++] == eT_br_cv_2)
        {
          nRecurs--;
          if (nRecurs == 0)
            break;
        }
      }
    }
    if (nStart <= nLast)
      break;
    if (pTokens[nStart] >= eT_float && pTokens[nStart] <= eT_int)
    {
      if (nStart+3 <= nLast) 
      {
        uint32 nName = pTokens[nStart+1];
        uint32 nN = pTokens[nStart+2];
        if (nN != eT_colon)
        {
          if (nN == eT_br_sq_1)
          {
            assert(pTokens[nStart+4] == eT_br_sq_2);
            if (pTokens[nStart+4] == eT_br_sq_2)
              nN = pTokens[nStart+5];
          }
        }
        if (nN == eT_colon)
          return nName;
        nStart += 3;
      }
      else
        break;
    }
    nStart++;
  }
  nStart = -1;
  return 0;
}

bool sIsAffectFuncs(CParserBin& Parser, uint32 nName)
{
  const uint32 *pTokens = Parser.GetTokens(0);
  int nStart = 0;
  int nLast = Parser.GetNumTokens()-1;

  while (nStart <= nLast)
  {
    if (pTokens[nStart] == eT_br_cv_1)
    {
      int nRecurs = 1;
      nStart++;
      int nBegin = nStart;
      while(nStart <= nLast)
      {
        if (pTokens[nStart++] == eT_br_cv_1)
          nRecurs++;
        else
        if (pTokens[nStart++] == eT_br_cv_2)
        {
          nRecurs--;
          if (nRecurs == 0)
            break;
        }
      }
      if (nStart <= nLast)
        break;
      int nPos = Parser.FindToken(nBegin, nStart, nName);
      if (nPos >= 0)
        return true;
    }
    nStart++;
  }
  return false;
}*/

void CHWShader_D3D::RemoveUnaffectedParameters_D3D10(CParserBin& Parser, SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars)
{
#ifdef DIRECT3D10
  int nPos = Parser.FindToken(0, Parser.m_Tokens.size()-1, eT_cbuffer);
  while (nPos >= 0)
  {
    uint32 nName = Parser.m_Tokens[nPos+1];
    if (nName == eT_PER_BATCH || nName == eT_PER_INSTANCE)
    {
      int nPosEnd = Parser.FindToken(nPos+3, Parser.m_Tokens.size()-1, eT_br_cv_2);
      assert(nPosEnd >= 0);
      int nPosN = Parser.FindToken(nPos+1, Parser.m_Tokens.size()-1, eT_br_cv_1);
      assert(nPosN >= 0);
      nPosN++;
      while (nPosN < nPosEnd)
      {
        uint32 nT = Parser.m_Tokens[nPosN+1];
        int nPosCode = Parser.FindToken(nPosEnd+1, Parser.m_Tokens.size()-1, nT);
        if (nPosCode < 0)
        {
          assert(nPosN > 0 && nPosN < Parser.m_Tokens.size());
          int i;
          if (InstBindVars.size())
          {
            CCryName nm = Parser.GetString(nT);
            for (i=0; i<InstBindVars.size(); i++)
            {
              SCGBind &b = InstBindVars[i];
              if (b.m_Name == nm)
                break;
            }
            if (i == InstBindVars.size())
              Parser.m_Tokens[nPosN] = eT_comment;
          }
          else
            Parser.m_Tokens[nPosN] = eT_comment;
        }
        nPosN = Parser.FindToken(nPosN+2, nPosEnd, eT_semicolumn);
        assert(nPosN >= 0);
        nPosN++;
      }
      nPos = Parser.FindToken(nPosEnd+1, Parser.m_Tokens.size()-1, eT_cbuffer);
    }
    else
      nPos = Parser.FindToken(nPos+2, Parser.m_Tokens.size()-1, eT_cbuffer);
  }
#else
  /*int nStart = 0;
  while (true)
  {
    uint32 nName = sFindVar(Parser, nStart);
    if (nStart < 0)
      break;
    bool bAffect = sIsAffectFuncs(Parser, nName);
    if (!bAffect)
      Parser.m_Tokens[nStart] = eT_comment;
    nStart++;
  }*/
#endif
}

struct SStructData
{
  uint32 m_nName;
  uint32 m_nTCs;
  int m_nPos;
};

void CHWShader_D3D::CorrectScriptEnums(CParserBin& Parser, SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars, FXShaderToken* Table)
{
  // correct enumeration of TEXCOORD# interpolators after preprocessing
  int nCur = 0;
  int nSize = Parser.m_Tokens.size();
  uint32 *pTokens = &Parser.m_Tokens[0];
  int nInstParam = 0;
  const uint32 Toks[] = {eT_TEXCOORDN, eT_TEXCOORDN_centroid, eT_unknown};
  std::vector<SStructData> SData;
  uint32 i;
  while (true)
  {
    nCur = Parser.FindToken(nCur, nSize-1, eT_struct);
    if (nCur < 0)
      break;
    int nLastStr = Parser.FindToken(nCur, nSize-1, eT_br_cv_2);
    assert(nLastStr >= 0);
    if (nLastStr < 0)
      break;
    bool bNested = false;
    for (i=0; i<SData.size(); i++)
    {
      SStructData& Data = SData[i];
      Data.m_nPos = Parser.FindToken(nCur, nLastStr, Data.m_nName);
      if (Data.m_nPos > 0)
        bNested = true;
    }
    uint32 nName = pTokens[nCur+1];
    int n = 0;
    while (nCur < nLastStr)
    {
      int nTN = Parser.FindToken(nCur, nLastStr, Toks);
      if (nTN < 0)
      {
        nCur = nLastStr+1;
        break;
      }
      if (bNested)
      {
        for (i=0; i<SData.size(); i++)
        {
          SStructData& Data = SData[i];
          if (Data.m_nPos > 0 && nTN > Data.m_nPos)
            n += Data.m_nTCs;
        }
      }
      assert(pTokens[nTN-1] == eT_colon);
      int nArrSize = 1;
      uint32 nTokName;
      if (pTokens[nTN-2] == eT_br_sq_2)
      {
        nArrSize = pTokens[nTN-3] - eT_0;
        assert(pTokens[nTN-4] == eT_br_sq_1);
        nTokName = pTokens[nTN-5];
      }
      else
      {
        uint32 nType = pTokens[nTN-3];
        assert(nType==eT_float || nType==eT_float2 || nType==eT_float3 || nType==eT_float4 || nType==eT_float4x4 || nType==eT_float3x4 || nType==eT_float2x4 || nType==eT_float3x3 || 
               nType==eT_half  || nType==eT_half2  || nType==eT_half3  || nType==eT_half4  || nType==eT_half4x4  || nType==eT_half3x4  || nType==eT_half2x4  || nType==eT_half3x3);
        if (nType == eT_float4x4 || nType == eT_half4x4)
          nArrSize = 4;
        else
        if (nType == eT_float3x4 || nType == eT_float3x3 || nType == eT_half3x4 || nType == eT_half3x3)
          nArrSize = 3;
        else
        if (nType == eT_float2x4 || nType == eT_half2x4)
          nArrSize = 2;
        nTokName = pTokens[nTN-2];
      }
      assert(nArrSize>0 && nArrSize<16);
//PS3 not a workaround, PS3 has no centroid filtering!



      EToken eT = (pTokens[nTN]==eT_TEXCOORDN) ? eT_TEXCOORD0 : eT_TEXCOORD0_centroid;

      n = min(n, 15); 

      pTokens[nTN] = n+eT;
      n += nArrSize;
      nCur = nTN+1;

      if (pInst->m_RTMask & (g_HWSR_MaskBit[HWSR_INSTANCING_ATTR] | g_HWSR_MaskBit[HWSR_INSTANCING_CONST]))
      {
        const char *szName = Parser.GetString(nTokName, *Table);
        if (!strnicmp(szName, "Inst", 4))
        {
          char newName[256];
          int nm = 0;
          while(szName[4+nm] > 0x20 && szName[4+nm] != '[')
          {
            newName[nm] = szName[4+nm];
            nm++;
          }
          newName[nm++] = 0;

          SCGBind bn;
          bn.m_dwBind = nInstParam;
          bn.m_nParameters = nArrSize;
          bn.m_Name = newName;
          InstBindVars.push_back(bn);

          nInstParam += nArrSize;
        }
      }
    }
    SStructData SD;
    SD.m_nName = nName;
    SD.m_nPos = -1;
    SD.m_nTCs = n;
    SData.push_back(SD);
  }
  pInst->m_nNumInstAttributes = nInstParam;
}

static int sFetchInst(uint32& nCur, uint32 *pTokens, uint32 nT, DynArray<uint32>& Parameter)
{
  while (true)
  {
    uint32 nTok = pTokens[nCur];
    if (nTok!=eT_br_rnd_1 && nTok!=eT_br_rnd_2 && nTok!=eT_comma)
      break;
    nCur++;
  }
  int nC = 0;
  Parameter.push_back(pTokens[nCur]);
  nCur++;
  while (pTokens[nCur] == eT_dot)
  {
    nC = 2;
    Parameter.push_back(pTokens[nCur]);
    Parameter.push_back(pTokens[nCur+1]);
    nCur += 2;
  }
  return nC;
}

static void sCR(TArray<char>& Text, int nLevel)
{
  Text.AddElem('\n');
  for (int i=0; i<nLevel; i++)
  {
    Text.AddElem(' ');
    Text.AddElem(' ');
  }
}

bool CHWShader_D3D::ConvertBinScriptToASCII(CParserBin& Parser, SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars, FXShaderToken* Table, TArray<char>& Text)
{
  uint32 i;
  bool bRes = true;

  /*if (pInst->m_RTMask == 0x2000020000680104)
  {
    TArray<char> TempTx;
    CParserBin::ConvertToAscii(&Parser.m_Tokens[0], Parser.m_Tokens.size(), *Table, TempTx);
    FILE *fp = gEnv->pCryPak->FOpen("inst.txt", "w");
    if (fp)
    {
      gEnv->pCryPak->FPrintf(fp, "%s", &TempTx[0]);
      gEnv->pCryPak->FClose (fp);
    }
  }*/
  uint32 *pTokens = &Parser.m_Tokens[0];
  uint32 nT = Parser.m_Tokens.size();
  const char *szPrev = " ";
  int nLevel = 0;
  for (i=0; i<nT; i++)
  {
    uint32 nToken = pTokens[i];
    if (nToken == 0)
    {
      Text.Copy("\n", 1);
      continue;
    }
    if (nToken == eT_skip)
    {
      i++;
      continue;
    }
    if (nToken == eT_skip_1)
    {
      while (i < nT)
      {
        nToken = pTokens[i];
        if (nToken == eT_skip_2)
          break;
        i++;
      }
      assert(i < nT);
      continue;
    }
    if (nToken == eT_fetchinst)
    {
      char str[256];
      i++;
      DynArray<uint32> ParamDst, ParamSrc;
      TArray<char> sParamDstFull, sParamDstName, sParamSrc;
      int nDst = sFetchInst(i, &Parser.m_Tokens[0], Parser.m_Tokens.size(), ParamDst);
      assert(Parser.m_Tokens[i] == eT_eq);
      if (Parser.m_Tokens[i] != eT_eq)
      { // Should never happen
        int n = CParserBin::FindToken(i, Parser.m_Tokens.size()-1, &Parser.m_Tokens[0], eT_semicolumn);
        if (n > 0)
          i = n+1;
        continue;
      }
      i++;
      int nSrc = sFetchInst(i, &Parser.m_Tokens[0], Parser.m_Tokens.size(), ParamSrc);
      CParserBin::ConvertToAscii(&ParamDst[0], ParamDst.size(), *Table, sParamDstFull);
      CParserBin::ConvertToAscii(&ParamDst[nDst], 1, *Table, sParamDstName);
      CParserBin::ConvertToAscii(&ParamSrc[nSrc], 1, *Table, sParamSrc);
      assert(strncmp(&sParamSrc[0], "Inst", 4) == 0);




































      if (pInst->m_RTMask & g_HWSR_MaskBit[HWSR_INSTANCING_CONST])
      {
        int nC = 0;
        int32 nn;
        for (nn=0; nn<InstBindVars.size(); nn++)
        {
          SCGBind& bn = InstBindVars[nn];
          if (!strcmp(bn.m_Name.c_str(), &sParamSrc[4]))
          {
            break;
          }
          nC += bn.m_nParameters;
        }
        assert(nn < InstBindVars.size());
        if (nn < InstBindVars.size())
        {
          DynArray<uint32> ParamInst;
          TArray<char> sParamInst;
          sFetchInst(i, &Parser.m_Tokens[0], Parser.m_Tokens.size(), ParamInst);
          CParserBin::ConvertToAscii(&ParamInst[0], ParamInst.size(), *Table, sParamInst);

          sprintf(str, "%s = _g_InstData[%s + %d];\n", &sParamDstFull[0], &sParamInst[0], nC);
          Text.Copy(str, strlen(str));
        }
      }
      else
      {
        sParamSrc.Free();
        CParserBin::ConvertToAscii(&ParamSrc[0], ParamSrc.size(), *Table, sParamSrc);
        sprintf(str, "%s = %s;\n", &sParamDstFull[0], &sParamSrc[0]);
        Text.Copy(str, strlen(str));
      }
      while(Parser.m_Tokens[i] != eT_semicolumn)
      {
        i++;
      }
      continue;
    }
    const char *szStr = CParserBin::GetString(nToken, *Table, false);
    assert(szStr);
    if (!szStr || !szStr[0])
    {
      assert(0);
      bRes = CParserBin::CorrectScript(pTokens, i, nT, Text);
    }
    else
    {
#if defined (_DEBUG) && !defined(PS3)
      int n = 0;
      while (szStr[n])
      {
        char c = szStr[n++];
        bool bASC = isascii(c);
        assert(bASC);
      }
#endif
      if (nToken == eT_semicolumn || nToken == eT_br_cv_1)
      {
        if (nToken == eT_br_cv_1)
        {
          sCR(Text, nLevel);
          nLevel++;
        }
        Text.Copy(szStr, strlen(szStr));
        if (nToken == eT_semicolumn)
        {
          if (i+1<nT && pTokens[i+1]==eT_br_cv_2)
            sCR(Text, nLevel-1);
          else
            sCR(Text, nLevel);
        }
        else
          if (i+1 < nT)
          {
            if (pTokens[i+1] < eT_br_rnd_1 || pTokens[i+1]>=eT_float)
              sCR(Text, nLevel);
          }
      }
      else
      {
        if (i+1 < nT)
        {
          if (Text.Num())
          {
            char cPrev = Text[Text.Num()-1];
            if (!sSkipChars[(uint8)cPrev] && !sSkipChars[(uint8)szStr[0]])
              Text.AddElem(' ');
          }
        }
        Text.Copy(szStr, strlen(szStr));
        if (nToken == eT_br_cv_2)
        {
          nLevel--;
          if (i+1<nT && pTokens[i+1]!=eT_semicolumn)
            sCR(Text, nLevel);
        }
      }
    }
  }
  Text.AddElem(0);

  return bRes;
}

void CHWShader_D3D::mfGetSrcFileName(char *srcName, int nSize)
{
  if (!m_NameSourceFX.empty())
  {
    strncpy(srcName, m_NameSourceFX.c_str(), nSize);
		srcName[nSize-1] = '\0';
    return;
  }
  strncpy(srcName, gRenDev->m_cEF.m_HWPath, nSize);
	srcName[nSize-1] = '\0';
  if (m_eSHClass == eHWSC_Vertex)
    strncat(srcName, "Declarations/CGVShaders/", nSize);
  else
  if (m_eSHClass == eHWSC_Pixel)
    strncat(srcName, "Declarations/CGPShaders/", nSize);
  else
    strncat(srcName, "Declarations/CGGShaders/", nSize);
  strncat(srcName, GetName(), nSize);
  strncat(srcName, ".crycg", nSize);
}

void CHWShader_D3D::mfGenName(SHWSInstance *pInst, char *dstname, int nSize, byte bType)
{
  if (bType)
    CHWShader::mfGenName(pInst->m_GLMask, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eClass, dstname, nSize, bType);
  else
    CHWShader::mfGenName(0, 0, 0, 0, 0, eHWSC_Max, dstname, nSize, bType);
}

void CHWShader_D3D::mfGetDstFileName(SHWSInstance *pInst, CHWShader_D3D *pSH, char *dstname, int nSize, byte bType)
{
  strncpy(dstname, gRenDev->m_cEF.m_ShadersCache, nSize);
	dstname[nSize-1] = '\0';

	//michaelg: Andrey: can we always use lower casing here?
	//PS3workaround : lower filename since it is checked for some directory strings to redirect to HOSTFS








	#define CGVSHADER_DIR "CGVShaders/"
	#define CGVSHADER_DEBUG_DIR "CGVShaders/Debug/"
	#define CGVSHADER_PENDING_DIR "CGVShaders/Pending/"
	#define CGPSHADER_DIR "CGPShaders/"
	#define CGPSHADER_DEBUG_DIR "CGPShaders/Debug/"
	#define CGPSHADER_PENDING_DIR "CGPShaders/Pending/"


  if (pSH->m_eSHClass == eHWSC_Vertex)
  {
    if (bType == 1 || bType == 4)
      strncat(dstname, CGVSHADER_DEBUG_DIR, nSize);
    else
    if (bType == 0)
      strncat(dstname, CGVSHADER_DIR, nSize);
    else
      if (bType == 2 || bType == 3)
      strncat(dstname, CGVSHADER_PENDING_DIR, nSize);
  }
  else
  if (pSH->m_eSHClass == eHWSC_Pixel)
  {
    if (bType == 1 || bType == 4)
      strncat(dstname, CGPSHADER_DEBUG_DIR, nSize);
    else
    if (bType == 0)
      strncat(dstname, CGPSHADER_DIR, nSize);
    else
    if (bType == 2 || bType == 3)
      strncat(dstname, CGPSHADER_PENDING_DIR, nSize);
  }
  else
  if (GEOMETRYSHADER_SUPPORT && pSH->m_eSHClass == eHWSC_Geometry)
  {
    if (bType == 1 || bType == 4)
      strncat(dstname, "CGGShaders/Debug/", nSize);
    else
    if (bType == 0)
      strncat(dstname, "CGGShaders/", nSize);
    else
      if (bType == 2 || bType == 3)
      strncat(dstname, "CGGShaders/Pending", nSize);
  }
  if (pSH->m_Flags & HWSG_SHARED)
  {
    char s[1024];
    sprintf(s, "_Shared@%s", pSH->m_EntryFunc.c_str());
    strncat(dstname, s, nSize);
    //strncat(dstname, GetName(), nSize);
  }
  else
    strncat(dstname, pSH->GetName(), nSize);
  if (bType == 2)
    strncat(dstname, "_out", nSize);
  if (bType == 0)
  {
    char *s = strchr(dstname, '(');
    if (s)
      s[0] = 0;
  }

  char szGenName[256];
  mfGenName(pInst, szGenName, 256, bType);

  strncat(dstname, szGenName, nSize);
}

//========================================================================================================
// Binary cache support

SShaderCache::~SShaderCache()
{
  CHWShader::m_ShaderCache.erase(m_Name);
  SAFE_DELETE(m_pRes[CACHE_USER]);
  SAFE_DELETE(m_pRes[CACHE_READONLY]);
}

int SShaderCache::Size()
{
  int nSize = sizeof(SShaderCache);

  if (m_pRes[0])
    nSize += m_pRes[0]->Size();
  if (m_pRes[1])
    nSize += m_pRes[1]->Size();
  nSize += m_DeviceShaders.size() * sizeof(SD3DShader);

  return nSize;
}

void SShaderCache::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject( m_pRes[0] );
	pSizer->AddObject( m_pRes[1] );
	pSizer->AddObject( m_DeviceShaders );	
}

SShaderCache *CHWShader::mfInitCache(const char *name, CHWShader *pSH, bool bCheckValid, uint32 CRC32, bool bDontUseUserFolder, bool bReadOnly)
{
//	LOADING_TIME_PROFILE_SECTION(iSystem);

  CHWShader_D3D *pSHHW = (CHWShader_D3D *)pSH;
  char nameCache[256];
  
  if (CRenderer::CV_r_shadersnocompile)
    bCheckValid = false;

  if (!name)
  {
    char namedst[256];
    pSHHW->mfGetDstFileName(pSHHW->m_pCurInst, pSHHW, namedst, 256, 0);
    fpStripExtension(namedst, nameCache);
    fpAddExtension(nameCache, ".fxcb");
    name = nameCache;
  }

  SShaderCache *pCache = NULL;
  FXShaderCacheItor it = m_ShaderCache.find(name);
  if (it != m_ShaderCache.end())
  {
    pCache = it->second;
    pCache->m_nRefCount++;
    if (pSHHW)
    {
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
          if (nCache[i] < 0 || !pCache->m_pRes[i])
            continue;
          bool bValid = true;
          if (pSHHW->m_Flags & HWSG_SHARED)
          {
            if (pCache->m_bValid[i])
              bValid = pCache->m_bValid[i];
            else
            {
              bValid = pSHHW->mfIsSharedCacheValid(pCache->m_pRes[i]);
              pCache->m_bValid[i] = bValid;
            }
          }
          else
            bValid = (pCache->m_Header[i].m_CRC32 == CRC32);
          if (!bValid)
          {
            SAFE_DELETE(pCache->m_pRes[i]);
          }
        }
        bool bValid = true;
        if ((!bReadOnly || bDontUseUserFolder) && !pCache->m_pRes[CACHE_USER])
          bValid = false;
        else
        if ((!bDontUseUserFolder || bReadOnly) && !pCache->m_pRes[CACHE_READONLY] && !pCache->m_pRes[CACHE_USER])
          bValid = false;
        if (!bValid)
        {
          mfOpenCacheFile(name, (float)FX_CACHE_VER, pCache, pSH, bCheckValid, CRC32, bDontUseUserFolder, bReadOnly);
        }
      }
    }
  }
  else
  {
    pCache = new SShaderCache;
    pCache->m_bPlatformD3D11 = CParserBin::m_bD3D11;
    pCache->m_bPlatformXenon = CParserBin::m_bXenon;
    pCache->m_bPlatformPS3 = CParserBin::m_bPS3;
    pCache->m_Name = name;
    mfOpenCacheFile(name, (float)FX_CACHE_VER, pCache, pSH, bCheckValid, CRC32, bDontUseUserFolder, bReadOnly);
    m_ShaderCache.insert(FXShaderCacheItor::value_type(name, pCache));
  }

  if (pSH && (pSH->m_Flags & HWSG_SHARED) && pCache->m_pRes[CACHE_USER])
    mfInsertSharedIdent(pCache, CRC32, pSH->m_NameSourceFX.c_str());

  return pCache;
}

#include "../Common/lzss.h"

SShaderCacheHeaderItem *CHWShader_D3D::mfGetCompressedItem(uint32 nFlags, uint32& nSize)
{
  SHWSInstance *pInst = m_pCurInst;
  /*if (pInst->m_GLMask == 0x200000000 && pInst->m_RTMask == 0x800000000000)
  {
    int nnn = 0;
  }*/
  char name[128];
  if (!strnicmp(m_EntryFunc.c_str(), "Common_", 7))
  {
    strcpy(name, "_shared@");
    strcat(name, m_EntryFunc.c_str());
  }
  else
  {
    strcpy(name, GetName());
    char *s = strchr(name, '(');
    if (s)
      s[0] = 0;
  }
  CCryNameCRC Name = name;
  FXCompressedShadersItor it = CHWShader::m_CompressedShaders.find(Name);
  if (it == CHWShader::m_CompressedShaders.end())
    return NULL;
  SHWActivatedShader *pAS = it->second;
  assert(pAS);
  if (!pAS)
    return NULL;
  mfGenName(pInst, name, 128, 1);
  Name = name;
  FXCompressedShaderRemapItor itR = pAS->m_Remap.find(Name);
  if (itR == pAS->m_Remap.end())
    return NULL;
  int nDevID = itR->second;
  FXCompressedShaderItor itS = pAS->m_CompressedShaders.find(nDevID);
  if (itS == pAS->m_CompressedShaders.end())
    return NULL;
  SCompressedData& CD = itS->second;
  byte *pData = new byte[CD.m_nSizeDecompressedShader];
  if (!pData)
    return NULL;
  pInst->m_DeviceObjectID = nDevID;
  Decodem(CD.m_pCompressedShader, pData, CD.m_nSizeCompressedShader);
  SShaderCacheHeaderItem *pIt = (SShaderCacheHeaderItem *)pData;
  if (CParserBin::m_bEndians)
    SwapEndian(*pIt, eBigEndian);
  nSize = CD.m_nSizeDecompressedShader;
  return pIt;
}

SShaderCacheHeaderItem *CHWShader_D3D::mfGetCacheItem(uint32& nFlags, uint32& nSize)
{
	LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);
  SHWSInstance *pInst = m_pCurInst;
  byte *pData = NULL;
  SShaderCacheHeaderItem *pIt = mfGetCompressedItem(nFlags, nSize);
  if (pIt)
  {
    pInst->m_bCompressed = true;
    return pIt;
  }
  nSize = 0;
  if (!m_pCache || (!m_pCache->m_pRes[CACHE_USER] && !m_pCache->m_pRes[CACHE_READONLY]))
    return NULL;
  CResFile *rf = NULL;
  SDirEntry *de;
  int i;
  for (i=0; i<2; i++)
  {
    de = NULL;
    rf = m_pCache->m_pRes[i];
    if (!rf)
      continue;
    char name[128];
    mfGenName(pInst, name, 128, 1);
    de = rf->mfGetEntry(name);
    if (de)
      break;
  }
  if (de)
  {
    pInst->m_nCache = i;
    pIt = NULL;
    nSize = rf->mfFileRead(de);
    pData = (byte *)rf->mfFileGetBuf(de);
    if (pData)
    {
      byte *pD = new byte[nSize];
      memcpy(pD, pData, nSize);
      pIt = (SShaderCacheHeaderItem *)pD;
      if (CParserBin::m_bEndians)
        SwapEndian(*pIt, eBigEndian);
      pInst->m_DeviceObjectID = de->offset;
      rf->mfFileClose(de);
    }
    if (i == CACHE_USER)
      nFlags |= HWSG_CACHE_USER;
    return pIt;
  }
  else
    return NULL;
}

bool CHWShader_D3D::mfAddCacheItem(SShaderCache *pCache, SShaderCacheHeaderItem *pItem, const byte *pData, int nLen, bool bFlush, CCryNameCRC Name)
{
  if (!pCache)
    return false;
  if (!pCache->m_pRes[CACHE_USER])
    return false;

  pItem->m_CRC32 = GetCRC32Gen().GetCRC32((const char *)pData, nLen, 0xffffffff);
  //CryLog("Size: %d: CRC: %x", nLen, pItem->m_CRC32);

  byte *pNew = new byte[sizeof(SShaderCacheHeaderItem)+nLen];
  SDirEntry de;
  de.offset = 0;
  if (CParserBin::m_bEndians)
  {
    SShaderCacheHeaderItem IT = *pItem;
    SwapEndian(IT, eBigEndian);
    memcpy(pNew, &IT, sizeof(SShaderCacheHeaderItem));
  }
  else
    memcpy(pNew, pItem, sizeof(SShaderCacheHeaderItem));
  memcpy(&pNew[sizeof(SShaderCacheHeaderItem)], pData, nLen);
  de.Name = Name;
  de.flags = RF_COMPRESS | RF_TEMPDATA;
  de.size = nLen+sizeof(SShaderCacheHeaderItem);
  de.pData = pNew;
  pCache->m_pRes[CACHE_USER]->mfFileAdd(&de);
  mfMarkCacheOptimised(false, pCache);
  if (bFlush)
    pCache->m_pRes[CACHE_USER]->mfFlush();

  return true;
}

bool CHWShader::mfMarkCacheOptimised(bool bOptimised, SShaderCache *pCache)
{
  if (!pCache)
    return false;
  if (pCache->m_Header[CACHE_USER].m_bOptimised == bOptimised)
    return true;
  if (!pCache->m_pRes[CACHE_USER])
    return false;
  CResFile *pRes = pCache->m_pRes[CACHE_USER];

  pCache->m_Header[CACHE_USER].m_bOptimised = bOptimised;
  CCryNameCRC nmHead = CShaderMan::s_cNameHEAD;
  SShaderCacheHeader hdTemp, *pHD;
  pHD = &pCache->m_Header[CACHE_USER];
  if (CParserBin::m_bEndians)
  {
    hdTemp = pCache->m_Header[CACHE_USER];
    SwapEndian(hdTemp, eBigEndian);
    pHD = &hdTemp;
  }
  pRes->mfFileWrite(nmHead, pHD);

  return true;
}

std::vector<SEmptyCombination> SEmptyCombination::Combinations;

bool CHWShader_D3D::mfAddEmptyCombination(CShader *pSH, uint64 nRT, uint64 nGL, uint32 nLT)
{
  CD3D9Renderer *rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

  SEmptyCombination Comb;
  Comb.nGLNew = m_nMaskGenShader;
  Comb.nRTNew = rRP.m_FlagsShader_RT & m_nMaskAnd_RT | m_nMaskOr_RT;
  Comb.nLTNew = rRP.m_FlagsShader_LT;
  Comb.nGLOrg = nGL;
  Comb.nRTOrg = nRT & m_nMaskAnd_RT | m_nMaskOr_RT;
  Comb.nLTOrg = nLT;
  Comb.nMD = rRP.m_FlagsShader_MD;
  Comb.nMDV = rRP.m_FlagsShader_MDV;
  if (m_eSHClass == eHWSC_Pixel)
  {
    Comb.nMD &= ~HWMD_TCMASK;
    Comb.nMDV = 0;
  }

  Comb.pShader = this;
  if (Comb.nRTNew!=Comb.nRTOrg || Comb.nGLNew!=Comb.nGLOrg || Comb.nLTNew!=Comb.nLTOrg)
    SEmptyCombination::Combinations.push_back(Comb);

  m_nMaskGenShader = nGL;

  return true;
}

bool CHWShader_D3D::mfStoreEmptyCombination(SEmptyCombination& Comb)
{
  if (!m_pCache || !m_pCache->m_pRes[CACHE_USER])
    return false;

  CResFile *rf = m_pCache->m_pRes[CACHE_USER];
  char nameOrg[128];
  char nameNew[128];
  SHWSInstance *pInstNew = mfGetInstance(Comb.nRTNew, Comb.nLTNew, Comb.nGLNew, Comb.nMD, Comb.nMDV, 0);
  mfGenName(pInstNew, nameNew, 128, 1);
  SDirEntry *deNew = rf->mfGetEntry(nameNew);
  //assert(deNew);
  if (!deNew)
    return false;

  SHWSInstance *pInstOrg = mfGetInstance(Comb.nRTOrg, Comb.nLTOrg, Comb.nGLOrg, Comb.nMD, Comb.nMDV, 0);
  mfGenName(pInstOrg, nameOrg, 128, 1);
  SDirEntry *deOrg = rf->mfGetEntry(nameOrg);
  if (deOrg)
  {
    if (deOrg->offset != deNew->offset)
    {
      deOrg->offset = -abs(deNew->offset);
      deOrg->flags |= RF_NOTSAVED;
    }
    return true;
  }
  SDirEntry de;
  de.Name = CCryNameCRC(nameOrg);
  de.flags = deNew->flags | RF_REFERENCE;
  de.size = deNew->size;
  de.offset = -abs(deNew->offset);
  rf->mfFileAdd(&de);

  return true;
}

bool CHWShader_D3D::mfFlushCacheFile()
{
  int i;

  for (i=0; i<m_Insts.size(); i++)
  {
    SHWSInstance *pInst = &m_Insts[i];
    if (pInst->m_Handle.m_bStatus == 2) // Fake
    {
      pInst->m_Handle.SetShader(NULL);
    }
  }
  return m_pCache && m_pCache->m_pRes[CACHE_USER] && m_pCache->m_pRes[CACHE_USER]->mfFlush();
}

#ifndef XENON
struct SData
{
  CCryNameCRC Name;
  int nSize;
  uint32 CRC;
  uint16 flags;
  int nOffset;
  byte *pData;
  byte bProcessed;
};
// Remove shader duplicates
bool CHWShader::mfOptimiseCacheFile(SShaderCache *pCache, bool bForce, SOptimiseStats *pStats)
{
  if (pCache->m_Header[CACHE_USER].m_bOptimised)
    return true;
  CResFile *pRes = pCache->m_pRes[CACHE_USER];
  pRes->mfFlush();
  std::vector<SDirEntry> *Dir = pRes->mfGetDirectory();
  std::vector<SData> Data;
  uint32 i, j;
  bool bNeedOptimise = true;
  if (pStats)
    pStats->nEntries += Dir->size();
  for (i=0; i<Dir->size(); i++)
  {
    SDirEntry *pDE = &(*Dir)[i];
    if (pDE->flags & RF_RES_$)
    {
      if (pDE->Name == CShaderMan::s_cNameHEAD)
        continue;
      SData d;
      d.nSize = pRes->mfFileRead(pDE);
      if (pStats)
        pStats->nTokenDataSize += d.nSize;
      d.pData = new byte[d.nSize];
      memcpy(d.pData, pDE->pData, d.nSize);
      d.bProcessed = 3;
      d.Name = pDE->Name;
      d.CRC = 0;
      d.nOffset = 0;
      d.flags = (short)pDE->flags;
      Data.push_back(d);
      continue;
    }
    SData d;
    d.flags = pDE->flags;
    d.nSize = pRes->mfFileRead(pDE);
    assert(d.nSize);
    if (!d.nSize)
      continue;
    d.nOffset = pDE->offset;
    d.pData = new byte[d.nSize];
    memcpy(d.pData, pDE->pData, d.nSize);
    SShaderCacheHeaderItem *pItem = (SShaderCacheHeaderItem *)d.pData;
    if (CParserBin::m_bEndians)
      SwapEndian(*pItem, eBigEndian);
    d.bProcessed = 0;
    d.Name = pDE->Name;
    d.CRC = pItem->m_CRC32;
    Data.push_back(d);
  }
#ifdef _DEBUG
  for (i=0; i<Data.size(); i++)
  {
    SData *pD1 = &Data[i];
    for (j=i+1; j<Data.size(); j++)
    {
      SData *pD2 = &Data[j];
      assert (pD1->Name != pD2->Name);
    }    
  }
#endif
  //FILE *fp = NULL;
  int nDevID = 0x10000000;
  int nOutFiles = Data.size();
  if (bNeedOptimise)
  {
    for (i=0; i<Data.size(); i++)
    {
      /*if (fp)
      {
        gEnv->pCryPak->FClose(fp);
        fp = NULL;
      }*/
      if (Data[i].bProcessed)
        continue;
      byte *pD = Data[i].pData;
      Data[i].bProcessed = 1;
      SShaderCacheHeaderItem *pItem = (SShaderCacheHeaderItem *)pD;
      Data[i].nOffset = nDevID++;
      byte *pSH = mfIgnoreBindsFromCache(pItem->m_nInstBinds, &pD[sizeof(SShaderCacheHeaderItem)]);
      int nSizeSh = (int)(Data[i].nSize - (pSH - pD));
      if (nSizeSh == 0)
        continue;
      assert(nSizeSh > 0);
      for (j=i+1; j<Data.size(); j++)
      {
        if (Data[j].bProcessed)
          continue;
        byte *pD1 = Data[j].pData;
        SShaderCacheHeaderItem *pItem1 = (SShaderCacheHeaderItem *)pD1;
        byte *pSH1 = mfIgnoreBindsFromCache(pItem1->m_nInstBinds, &pD1[sizeof(SShaderCacheHeaderItem)]);
        int nSizeSh1 = (int)(Data[j].nSize - (pSH1 - pD1));
        if (nSizeSh1 == 0)
        {
          Data[j].bProcessed = 1;
          Data[j].nOffset = nDevID++;
          continue;
        }
        if (nSizeSh != nSizeSh1)
          continue;
        if (Data[i].CRC != Data[j].CRC)
          continue;
        if (!memcmp(pSH, pSH1, nSizeSh))
        {
          if (!memcmp(&pD[sizeof(SShaderCacheHeaderItem)], &pD1[sizeof(SShaderCacheHeaderItem)], pSH1 - pD1))
          {
            /*if (!fp && CRenderer::CV_r_shaderscacheoptimiselog)
            {
              char name[256];
              sprintf(name, "Optimise/%s/%s.cache", pRes->mfGetFileName(), Data[i].Name.c_str());
              fp = gEnv->pCryPak->FOpen(name, "w");
            }*/
            Data[j].nOffset = Data[i].nOffset;
            Data[j].bProcessed = 2;
            nOutFiles--;
            //if (fp)
            //  gEnv->pCryPak->FPrintf(fp, "%s\n", Data[j].Name.c_str());
          }
        }
      }
    }
  }
  /*if (fp)
  {
    gEnv->pCryPak->FClose(fp);
    fp = NULL;
  }*/
  if (nOutFiles != Data.size())
  {
    iLog->Log(" Optimising shaders resource '%s' (%d items)...", pCache->m_Name.c_str(), Data.size()-1);

    pRes->mfClose();
    pRes->mfOpen(RA_CREATE | (CParserBin::m_bEndians * RA_ENDIANS));

    float fVersion = (float)FX_CACHE_VER;
    SDirEntry de;
    SShaderCacheHeader hd, hdTemp, *pHD;
    ZeroStruct(hd);
    de.Name = CShaderMan::s_cNameHEAD;
    de.flags = RF_RES_$HEAD;
    de.size = sizeof(SShaderCacheHeader);
    hd.m_SizeOf = sizeof(SShaderCacheHeader);
    hd.m_MinorVer = (int)(((float)fVersion - (float)(int)fVersion)*10.1f);
    hd.m_MajorVer = (int)fVersion;
    hd.m_CRC32 = pCache->m_Header[CACHE_USER].m_CRC32;
    sprintf(hd.m_szVer, "Ver: %.1f", fVersion);
    pHD = &hd;
    if (CParserBin::m_bEndians)
    {
      hdTemp = hd;
      SwapEndian(hdTemp, eBigEndian);
      pHD = &hdTemp;
    }
    de.pData = pHD;
    pRes->mfFileAdd(&de);
    pRes->mfFlush();

    for (i=0; i<Data.size(); i++)
    {
      SData *pD = &Data[i];
      SShaderCacheHeaderItem *pItem = (SShaderCacheHeaderItem *)pD->pData;

      SDirEntry de;
      de.Name = pD->Name;
      de.flags = pD->flags;
      de.size = pD->nSize;
      if (pD->bProcessed == 1)
      {
        byte *pNew = new byte[de.size];
        memcpy(pNew, pD->pData, pD->nSize);
        de.pData = pNew;
        de.offset = pD->nOffset;
        SShaderCacheHeaderItem *pItem1 = (SShaderCacheHeaderItem *)pNew;
        if (CParserBin::m_bEndians)
          SwapEndian(*pItem1, eBigEndian);
        de.flags |= RF_TEMPDATA | RF_COMPRESS;
        if (pStats)
        {
          pStats->nSizeUncompressed += de.size;
          pStats->nUniqueEntries++;
        }
      }
      else
      if (pD->bProcessed != 3)
      {
        de.flags |= RF_COMPRESS | RF_REFERENCE;
        de.offset = -pD->nOffset;
      }
      else
        de.pData = pD->pData;
      pRes->mfFileAdd(&de);
    }
  }

  if (nOutFiles != Data.size())
    iLog->Log("  -- Removed %d duplicated shaders", Data.size()-nOutFiles);

  float fVersion = (float)FX_CACHE_VER;
  sprintf(pCache->m_Header[CACHE_USER].m_szVer, "Ver: %.1f", fVersion);

  Data.clear();
  mfMarkCacheOptimised(true, pCache);
  int nSizeCompr = pRes->mfFlush();
  if (pStats)
    pStats->nSizeCompressed += nSizeCompr;

  for (i=0; i<Data.size(); i++)
  {
    SData *pD = &Data[i];
    delete [] pD->pData;
  }

  if (pStats)
    CryLog("  -- Shader cache stats: Entries: %d, Unique Entries: %d, Size: %d, Compressed Size: %d, Token data size: %d", pStats->nEntries, pStats->nUniqueEntries, pStats->nSizeUncompressed, pStats->nSizeCompressed, pStats->nTokenDataSize);

  return true;
}
#endif

bool CHWShader::mfIsSharedCacheValid(CResFile *pRF)
{
  std::vector<SDirEntry> *Dir = pRF->mfGetDirectory();
  uint32 i;
  for (i=0; i<Dir->size(); i++)
  {
    SDirEntry *pDE = &(*Dir)[i];
    if (!(pDE->flags & RF_RES_$NAME))
      continue;
    CCryNameCRC NM = CParserBin::GetPlatformSpecName(pDE->Name);
    FXShaderBinPathItor it = gRenDev->m_cEF.m_Bin.m_BinPaths.find(NM);
    if (it == gRenDev->m_cEF.m_Bin.m_BinPaths.end())
    {
      assert(0);
      return false;
    }
    const char *szBinPath = it->second.c_str();
    FILE *fp = gEnv->pCryPak->FOpen(szBinPath, "rb");
    if (!fp)
    {
      assert(0);
      return false;
    }
    SShaderBinHeader Header;
    gEnv->pCryPak->FRead((byte *)&Header, sizeof(Header), fp);
    if (CParserBin::m_bEndians)
      SwapEndian(Header, eBigEndian);
    uint32 CRC32 = Header.m_CRC32;
    pRF->mfFileRead(pDE);
    byte *pData = (byte *)pRF->mfFileGetBuf(pDE);
    uint32 CRC32Cache = *(uint32 *)&pData[0];
    if (CParserBin::m_bEndians)
      SwapEndian(CRC32Cache, eBigEndian);
    gEnv->pCryPak->FClose(fp);
    if (CRC32 != CRC32Cache)
      return false;
  }
  return true;
}

bool CHWShader::mfInsertSharedIdent(SShaderCache *pCache, uint32 CRC32, const char *szNameFX)
{
  if (!pCache)
    return false;
  
  CResFile *pRF = pCache->m_pRes[CACHE_USER];
  if(!pRF)//happened on PS3 
    return false;
  std::vector<SDirEntry> *Dir = pRF->mfGetDirectory();
  uint32 i;

  char nm[256], nmF[256];
  nm[0] = 0;
  _splitpath(szNameFX, NULL, NULL, nm, NULL);
  if (!nm[0])
    return false;
  sprintf(nmF, "$%s", nm);
  CCryNameCRC NM = nmF;
  for (i=0; i<Dir->size(); i++)
  {
    SDirEntry *pDE = &(*Dir)[i];
    if (pDE->Name == NM)
    {
      assert(pDE->flags & RF_RES_$NAME);
      return true;
    }
  }
  byte *pNewData = new byte [sizeof(uint32)];
  if (CParserBin::m_bEndians)
    SwapEndian(CRC32, eBigEndian);
  *(uint32 *)pNewData = CRC32;
  SDirEntry de;
  de.Name = NM;
  de.flags = RF_RES_$NAME | RF_TEMPDATA;
  de.size = sizeof(uint32);
  de.pData = pNewData;
  pRF->mfFileAdd(&de);

  return true;
}

int __cdecl sSort( const VOID* arg1, const VOID* arg2 )
{
  SDirEntry **pi1 = (SDirEntry **)arg1;
  SDirEntry **pi2 = (SDirEntry **)arg2;
  SDirEntry *ti1 = *pi1;
  SDirEntry *ti2 = *pi2;
  if (ti1->Name<ti2->Name)
    return -1;
  if (ti1->Name==ti2->Name)
    return 0;
  return 1;
}

bool CHWShader::_OpenCacheFile(float fVersion, SShaderCache *pCache, CHWShader *pSH, bool bCheckValid, uint32 CRC32, int nCache, CResFile *pRF, bool bReadOnly)
{
	assert(nCache == CACHE_USER || nCache == CACHE_READONLY);

  SShaderCacheHeader hd;
  ZeroStruct(hd);
  bool bValid = true;
  CHWShader_D3D *pSHHW = (CHWShader_D3D *)pSH;

  if (!pRF->mfOpen(RA_READ | (CParserBin::m_bEndians ? RA_ENDIANS : 0)))
  {
    pRF->mfClose();
    bValid = false;
  }
  else
  {
    if (pSH)
    {
      if (pSH->m_Flags & HWSG_SHARED)
      {
        if (bCheckValid)
          bValid = mfIsSharedCacheValid(pRF);
      }
      if (!bValid && CRenderer::CV_r_shadersdebug==2)
      {
        LogWarning("WARNING: Shader cache shared '%s' mismatch", pRF->mfGetFileName());
      }
    }
    if (bValid)
    {
      //if (!strnicmp(pRF->mfGetFileName(), "Cloth", 5))
      /*{
        TArray<SDirEntry *> Dir;
        pRF->mfGetDirectory(Dir);
        FILE *fp = gEnv->pCryPak->FOpen("Cloth.txt", "w");
        qsort(&Dir[0], Dir.Num(), sizeof(SDirEntry *), sSort);
        for (int i=0; i<Dir.Num(); i++)
        {
          SDirEntry *pDE = Dir[i];
          if (!strcmp(pDE->Name.c_str(), "(GL210)(RT2)(ps_2_0)"))
          {
            int nnn = 0;
          }
          gEnv->pCryPak->FPrintf(fp, "%s\t\t%d\n", pDE->Name.c_str(), pDE->ref>0 ? 0 : pDE->size);
        }
        gEnv->pCryPak->FClose(fp);
      }*/
      pRF->mfFileSeek(CShaderMan::s_cNameHEAD, 0, SEEK_SET);
      pRF->mfFileRead2(CShaderMan::s_cNameHEAD, sizeof(SShaderCacheHeader), &hd);
      if (CParserBin::m_bEndians)
        SwapEndian(hd, eBigEndian);
      if (bCheckValid)
      {
        if (hd.m_SizeOf != sizeof(SShaderCacheHeader))
          bValid = false;
        else
        if (fVersion && (hd.m_MajorVer != (int)fVersion || hd.m_MinorVer != (int)(((float)fVersion - (float)(int)fVersion)*10.1f)))
          bValid = false;
        if (!bValid && (CRenderer::CV_r_shadersdebug==2 || nCache == CACHE_READONLY))
        {
          LogWarning("WARNING: Shader cache '%s' version mismatch (Cache: %s, Expected: %.1f)", pRF->mfGetFileName(), hd.m_szVer, fVersion);
        }
        if (pSH)
        {
          if (bValid && hd.m_CRC32 != pSHHW->m_CRC32)
          {
            if (!(pSH->m_Flags & HWSG_SHARED))
            {
              bValid = false;
              if (CRenderer::CV_r_shadersdebug==2 && (CRenderer::CV_r_shadersdebug==2 || nCache == CACHE_READONLY))
              {
                LogWarning("WARNING: Shader cache '%s' CRC mismatch", pRF->mfGetFileName());
              }
            }
          }
        }
      }
    }

    if (nCache == CACHE_USER)
    {
      pRF->mfClose();
      if (bValid)
      {
        int nAcc = !CRenderer::CV_r_shadersnocompile ? (RA_READ|RA_WRITE) : RA_READ;
				if (!pRF->mfOpen(nAcc|(CParserBin::m_bEndians ? RA_ENDIANS : 0)))
        {
          pRF->mfClose();
          bValid = false;
        }
      }
    }
  }
  if (!bValid && bCheckValid)
  {
    if (nCache == CACHE_USER && !bReadOnly)
    {
			if (!pRF->mfOpen(RA_CREATE|(CParserBin::m_bEndians ? RA_ENDIANS : 0)))			
        return false;
			
      SDirEntry de;
      de.Name = CShaderMan::s_cNameHEAD;
      de.flags = RF_RES_$HEAD;
      de.size = sizeof(SShaderCacheHeader);
      hd.m_SizeOf = sizeof(SShaderCacheHeader);
      hd.m_MinorVer = (int)(((float)fVersion - (float)(int)fVersion)*10.1f);
      hd.m_MajorVer = (int)fVersion;
      hd.m_CRC32 = CRC32;
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

      if (pSHHW)
        pRF->mfFlush();
      pCache->m_bNeedPrecache = true;
      bValid = true;
    }
    else
    {
      SAFE_DELETE(pRF);
    }
  }	
  pCache->m_pRes[nCache] = pRF;
  pCache->m_Header[nCache] = hd;
  pCache->m_bReadOnly[nCache] = bReadOnly;

  return bValid;
}

bool CHWShader::mfOpenCacheFile(const char *szName, float fVersion, SShaderCache *pCache, CHWShader *pSH, bool bCheckValid, uint32 CRC32, bool bDontUseUserFolder, bool bReadOnly)
{
  CResFile *rfRO = new CResFile(szName);
  bool bValidRO = _OpenCacheFile(fVersion, pCache, pSH, bCheckValid, CRC32, bDontUseUserFolder ? CACHE_USER : CACHE_READONLY, rfRO, bDontUseUserFolder ? false : bReadOnly);

  bool bValidUser = false;
  CResFile *rfUser;
  if (!bDontUseUserFolder)
  {
		stack_string szUser = stack_string(gRenDev->m_cEF.m_szUserPath.c_str()) + stack_string(szName);
    rfUser = new CResFile(szUser.c_str());
    bValidUser = _OpenCacheFile(fVersion, pCache, pSH, bCheckValid, CRC32, CACHE_USER, rfUser, bReadOnly);
  }

  return (bValidRO || bValidUser);
}

byte *CHWShader_D3D::mfBindsToCache(SHWSInstance *pInst, DynArray<SCGBind>* Binds, int nParams, byte *pP)
{
  int i;
  for (i=0; i<nParams; i++)
  {
    SCGBind *cgb = &(*Binds)[i];
    SShaderCacheHeaderItemVar *pVar = (SShaderCacheHeaderItemVar *)pP;
    pVar->m_nCount = cgb->m_nParameters;
    pVar->m_Reg = cgb->m_dwBind;
    int len = strlen(cgb->m_Name.c_str())+1;
    memcpy(pVar->m_Name,  cgb->m_Name.c_str(), len);
    pP += sizeof(SShaderCacheHeaderItemVar)-MAX_VAR_NAME+len;
  }
  return pP;
}

byte *CHWShader_D3D::mfBindsFromCache(DynArray<SCGBind>*& Binds, int nParams, byte *pP)
{
  int i;
  for (i=0; i<nParams; i++)
  {
    if (!Binds)
      Binds = new DynArray<SCGBind>;
    SCGBind cgb;
    SShaderCacheHeaderItemVar *pVar = (SShaderCacheHeaderItemVar *)pP;
    cgb.m_nParameters = pVar->m_nCount;
    cgb.m_Name = pVar->m_Name;
    cgb.m_dwBind = pVar->m_Reg;
    Binds->push_back(cgb);
    pP += sizeof(SShaderCacheHeaderItemVar)-MAX_VAR_NAME+strlen(pVar->m_Name)+1;
  }
  return pP;
}

byte *CHWShader::mfIgnoreBindsFromCache(int nParams, byte *pP)
{
  int i;
  for (i=0; i<nParams; i++)
  {
    SShaderCacheHeaderItemVar *pVar = (SShaderCacheHeaderItemVar *)pP;
    pP += sizeof(SShaderCacheHeaderItemVar)-MAX_VAR_NAME+strlen(pVar->m_Name)+1;
  }
  return pP;
}

bool CHWShader_D3D::mfUploadHW(SHWSInstance *pInst, byte *pBuf, uint32 nSize, CShader *pSH, uint32 nFlags)
{
  PROFILE_FRAME(Shader_mfUploadHW);

	const char *sHwShaderName = _HELP("Vertex Shader");
	if (m_eSHClass == eHWSC_Pixel)
		sHwShaderName = _HELP("Pixel Shader");
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_D3D, 0, "D3D HW %s",sHwShaderName );

  HRESULT hr = S_OK;
  if (!pInst->m_Handle.m_pShader)
    pInst->m_Handle.SetShader(new SD3DShader);

  if ((m_eSHClass == eHWSC_Vertex) && (!(nFlags & HWSF_PRECACHE) || gRenDev->m_cEF.m_bActivatePhase) && !pInst->m_bFallback)
    mfUpdateFXVertexFormat(pInst, pSH);

  pInst->m_nDataSize = nSize;
  if (m_eSHClass == eHWSC_Pixel)
    m_nDevicePSDataSize += nSize;
  else
    m_nDeviceVSDataSize += nSize;






#ifdef WIN32
  if(CRenderer::m_pDirectBee)
    CRenderer::m_pDirectBee->PushName(GetName());
#endif
#if defined (DIRECT3D9) || defined (OPENGL)










	assert(pInst->m_Handle.m_pShader);
  if (m_eSHClass == eHWSC_Pixel)
    hr = gRenDev->m_pRT->RC_CreatePixelShader((DWORD*)pBuf, &pInst->m_Handle.m_pShader->m_pHandle);
  else
    hr = gRenDev->m_pRT->RC_CreateVertexShader((DWORD*)pBuf, &pInst->m_Handle.m_pShader->m_pHandle, pInst);




#elif defined (DIRECT3D10)
  if (m_eSHClass == eHWSC_Pixel)
    hr = gcpRendD3D->GetD3DDevice()->CreatePixelShader((DWORD*)pBuf, nSize, NULL, (ID3D11PixelShader **)&pInst->m_Handle.m_pShader->m_pHandle);
  else
  if (m_eSHClass == eHWSC_Vertex)
    hr = gcpRendD3D->GetD3DDevice()->CreateVertexShader((DWORD*)pBuf, nSize, NULL, (ID3D11VertexShader **)&pInst->m_Handle.m_pShader->m_pHandle);
  else
  if (GEOMETRYSHADER_SUPPORT && m_eSHClass == eHWSC_Geometry)
    hr = gcpRendD3D->GetD3DDevice()->CreateGeometryShader((DWORD*)pBuf, nSize, NULL, (ID3D11GeometryShader **)&pInst->m_Handle.m_pShader->m_pHandle);
#endif

#ifdef WIN32
  if(CRenderer::m_pDirectBee)
    CRenderer::m_pDirectBee->PushName();
#endif

  return (hr == S_OK);
}

bool CHWShader_D3D::mfUploadHW(LPD3DXBUFFER pShader, SHWSInstance *pInst, CShader *pSH, uint32 nFlags)
{
  bool bResult = true;
  if (m_eSHClass == eHWSC_Vertex && !pInst->m_bFallback)
    mfUpdateFXVertexFormat(pInst, pSH);
  if (pShader && !(m_Flags & HWSG_PRECACHEPHASE))
  {
    DWORD *pCode = (DWORD*)pShader->GetBufferPointer();
    if (gcpRendD3D->m_cEF.m_nCombinationsProcess>=0 && !gcpRendD3D->m_cEF.m_bActivatePhase)
    {
      pInst->m_Handle.SetFake();
    }
    else
    {
      bResult = mfUploadHW(pInst, (byte *)pCode, pShader->GetBufferSize(), pSH, nFlags);
#if defined (DIRECT3D10)
      if (m_eSHClass == eHWSC_Vertex)
      {
        int nSize = pShader->GetBufferSize();
        pInst->m_pShaderData = new byte[nSize];
        pInst->m_nShaderByteCodeSize = nSize;
        memcpy(pInst->m_pShaderData, pCode, nSize);
      }
#endif
    }
    if (!bResult)
    {
      if (m_eSHClass == eHWSC_Vertex)



        Warning("CHWShader_D3D::mfUploadHW: Could not create vertex shader '%s'(0x%I64x)\n", GetName(), pInst->m_GLMask);

      else
      if (m_eSHClass == eHWSC_Pixel)



        Warning("CHWShader_D3D::mfUploadHW: Could not create pixel shader '%s'(0x%I64x)\n", GetName(), pInst->m_GLMask);

      else
      if (GEOMETRYSHADER_SUPPORT && m_eSHClass == eHWSC_Geometry)



        Warning("CHWShader_D3D::mfUploadHW: Could not create geometry shader '%s'(0x%I64x)\n", GetName(), pInst->m_GLMask);

    }
  }
  return bResult;
}


















bool CHWShader_D3D::mfActivateCacheItem(SShaderCacheHeaderItem *pItem, uint32 nSize, uint32 nFlags)
{
  SHWSInstance *pInst = m_pCurInst;
  byte *pData = (byte *)pItem;
  pData += sizeof(SShaderCacheHeaderItem);
  byte *pBuf = pData;
  DynArray<SCGBind> *pInstBinds = NULL;
  pInst->Release(m_pCache, false);
  pBuf = mfBindsFromCache(pInstBinds, pItem->m_nInstBinds, pBuf);
  nSize -= (uint32)(pBuf - (byte *)pItem);
  pInst->m_eClass = (EHWShaderClass)pItem->m_Class;
  pInst->m_nVertexFormat = pItem->m_nVertexFormat;
  pInst->m_nInstructions = pItem->m_nInstructions;
  assert(pInst->m_DeviceObjectID > 0);
  pInst->m_VStreamMask_Decl = pItem->m_StreamMask_Decl;
  pInst->m_VStreamMask_Stream = pItem->m_StreamMask_Stream;
  bool bResult = true;
  SD3DShader *pHandle = NULL;
  SShaderCache *pCache = m_pCache;
  if (!(nFlags & HWSG_CACHE_USER))
  {
    FXDeviceShaderItor it = pCache->m_DeviceShaders.find(pInst->m_DeviceObjectID);
    if (it != pCache->m_DeviceShaders.end())
      pHandle = it->second;
  }
  HRESULT hr = S_OK;
  if (pHandle)
  {
    pInst->m_Handle.SetShader(pHandle);
    pInst->m_Handle.AddRef();






















    if ((m_eSHClass == eHWSC_Vertex) && (!(nFlags & HWSF_PRECACHE) || gRenDev->m_cEF.m_bActivatePhase) && !pInst->m_bFallback)
      mfUpdateFXVertexFormat(pInst, gRenDev->m_RP.m_pShader);
  }
  else
  {
    if (gcpRendD3D->m_cEF.m_nCombinationsProcess>0 && !gcpRendD3D->m_cEF.m_bActivatePhase)
    {
      pInst->m_Handle.SetFake();
    }
    else
    {






















      bResult = mfUploadHW(pInst, pBuf, nSize, gRenDev->m_RP.m_pShader, nFlags);
    }
    if (!bResult)
    {
      SAFE_DELETE(pInstBinds);
      assert(!"Shader creation error");
      iLog->Log("WARNING: cannot create shader '%s' (FX: %s)", m_EntryFunc.c_str(), GetName());
      return true;
    }
    pCache->m_DeviceShaders.insert(FXDeviceShaderItor::value_type(pInst->m_DeviceObjectID, pInst->m_Handle.m_pShader));
  }
  LPD3DXCONSTANTTABLE pConstantTable = NULL;
#if defined (DIRECT3D9) || defined (OPENGL)
  hr = D3DXGetShaderConstantTable((DWORD *)pBuf, &pConstantTable);
 
#elif defined (DIRECT3D10)
  ID3D11ShaderReflection *pShaderReflection;
	hr = D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
  if (SUCCEEDED(hr))
    pConstantTable = (LPD3DXCONSTANTTABLE)pShaderReflection;	
  if (m_eSHClass == eHWSC_Vertex || gRenDev->IsEditorMode())
  {
    pInst->m_pShaderData = new byte[nSize];
    pInst->m_nShaderByteCodeSize = nSize;
    memcpy(pInst->m_pShaderData, pBuf, nSize);
  }
#endif
  assert(hr == S_OK);
  bResult &= (hr == S_OK);
  if (pConstantTable)
    mfCreateBinds(pInst, pConstantTable, pBuf, nSize);
#ifdef OPENGL
  int i;
  if (pInst->m_pBindVars)
  {
    for (i=0; i<pInst->m_pBindVars->size(); i++)
    {
      D3DXGetSHParamHandle(pInst->m_Handle.m_pHandle, &(*pInst->m_pBindVars)[i]);
    }
  }
  if (pInstBinds)
  {
    for (i=0; i<pInstBinds->size(); i++)
    {
      D3DXGetSHParamHandle(pInst->m_Handle.m_pHandle, &(*pInstBinds)[i]);
    }
  }
#endif

  mfGatherFXParameters(pInst, &pInst->m_pBindVars, pInstBinds, this, 0, gRenDev->m_RP.m_pShader);
  SAFE_DELETE(pInstBinds);
#if defined (DIRECT3D9) || defined (OPENGL)
  SAFE_RELEASE(pConstantTable);
#elif defined (DIRECT3D10)
  SAFE_RELEASE(pShaderReflection);
#endif

  return bResult;
}

/*CHWShader_D3D::SHWSInstance *g_pInst;
CHWShader_D3D::SHWSInstance g_Inst;
CHWShader_D3D::SHWSInstance *g_pInst0;
CHWShader_D3D::SHWSInstance g_Inst0;
CHWShader_D3D *g_pSH;*/

bool CHWShader_D3D::mfCreateCacheItem(SHWSInstance *pInst, DynArray<SCGBind>& InstBinds, byte *pData, int nLen, CHWShader_D3D *pSH, bool bShaderThread)
{
  if (!pSH->m_pCache || !pSH->m_pCache->m_pRes[CACHE_USER])
  {
    if (pSH->m_pCache)
      pSH->m_pCache->m_nRefCount--;
    pSH->m_pCache = mfInitCache(NULL, pSH, true, pSH->m_CRC32, false, false);
  }
  assert(pSH->m_pCache);
  if (!pSH->m_pCache || !pSH->m_pCache->m_pRes[CACHE_USER])
    return false;

  DynArray<BYTE> NewDataB;
  DynArray<DWORD> NewDataDW;
  bool bNeedConvert = false;
#if defined (DIRECT3D9) || defined (PS3)
  if (CParserBin::m_bD3D11)
    bNeedConvert = true;
#elif defined (DIRECT3D10)
  if (!CParserBin::m_bD3D11)
    bNeedConvert = true;
#endif
  if (bNeedConvert && pData && nLen)
  {
    bool bUseBytes = true;
    char *sSrc = (char *)pData;
    sSrc[nLen-1] = 0;
    char *sS = strstr(sSrc, "// Approximately");
    char *sB = NULL;
    if (sS)
    {
      pInst->m_nInstructions = atoi(&sS[17]);
      sB = strstr(sS, "const BYTE g_");
      if (!sB)
      {
        sB = strstr(sS, "const DWORD g_");
        if (sB)
          bUseBytes = false;
      }
    }
    assert(sB);
    if (!sB)
      return false;
    sB = strchr(sB, '{');
    assert(sB);
    sB++;
    while(true)
    {
      char val[16];
      shFill(&sB, val, 16);
      if (!bUseBytes && val[0] != '0' && val[1] != 'x')
        break;
      if (bUseBytes && !isdigit((unsigned)val[0]))
        break;
      DWORD dw = bUseBytes ? shGetInt(val) : shGetHex(&val[2]);
      if (bUseBytes)
        NewDataB.push_back((BYTE)dw);
      else
        NewDataDW.push_back(dw);
    }

    if (bUseBytes)
    {
      pData = (byte *)&NewDataB[0];
      nLen = NewDataB.size()*sizeof(BYTE);
    }
    else
    {
      pData = (byte *)&NewDataDW[0];
      nLen = NewDataDW.size()*sizeof(DWORD);
    }
  }


  SShaderCacheHeaderItem h;
  h.m_nInstBinds = InstBinds.size();
  h.m_nInstructions = pInst->m_nInstructions;
  h.m_nVertexFormat = pInst->m_nVertexFormat;
  h.m_Class = pData ? pInst->m_eClass : 255;
  h.m_StreamMask_Decl = pInst->m_VStreamMask_Decl;
  h.m_StreamMask_Stream = (byte)pInst->m_VStreamMask_Stream;
  int nNewSize = (h.m_nInstBinds)*sizeof(SShaderCacheHeaderItemVar)+nLen;
  byte *pNewData = new byte [nNewSize];
  byte *pP = pNewData;
  pP = mfBindsToCache(pInst, &InstBinds, h.m_nInstBinds, pP);
  memcpy(pP, pData, nLen);
  pP += nLen;
  char name[256];
  mfGenName(pInst, name, 256, 1);
  CCryNameCRC nm = CCryNameCRC(name);
  bool bRes = mfAddCacheItem(pSH->m_pCache, &h, pNewData, (int)(pP-pNewData), false, nm);
  SAFE_DELETE_ARRAY(pNewData);
  if (gRenDev->m_cEF.m_bActivatePhase || (!(pSH->m_Flags & HWSG_PRECACHEPHASE) && gRenDev->m_cEF.m_nCombinationsProcess <= 0))
  {
    if (!gRenDev->m_cEF.m_bActivatePhase)
    {
#if !defined(PS3)
      if (bShaderThread && false)
      {
        if (pInst->m_pAsync)
          pInst->m_pAsync->m_bPendedFlush = true;
      }
      else
#endif
        pSH->mfFlushCacheFile();
    }
    strcpy(name, pSH->GetName());
    char *s = strchr(name, '(');
    if (s)
      s[0] = 0;
    if (!bShaderThread || true)
    {
      byte bStore = 1;
      if (pSH->m_Flags & HWSG_FP_EMULATION)
        bStore = 2;
      gRenDev->m_cEF.mfInsertNewCombination(pSH->m_nMaskGenFX, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eClass, name, 0, NULL, bStore);
    }
  }
  pInst->m_nCache = CACHE_USER;

  return bRes;
}

//============================================================================

void CHWShader_D3D::mfSaveCGFile(const char *scr, const char *path)
{
  if (CRenderer::CV_r_shadersdebug < 1 && CRenderer::CV_r_shadersuserfolder)
    return;
  char name[1024];
  if (path && path[0])
  {



#if defined(__GNUC__)
    sprintf(name, "%s/%s(LT%x)@(RT%llx)(MD%x)(MDV%x)(GL%llx).cg", path, GetName(), m_pCurInst->m_LightMask, m_pCurInst->m_RTMask, m_pCurInst->m_MDMask, m_pCurInst->m_MDVMask, m_pCurInst->m_GLMask);
#else
    sprintf(name, "%s/%s(LT%x)/(RT%I64x)(MD%x)(MDV%x)(GL%I64x).cg", path, GetName(), m_pCurInst->m_LightMask, m_pCurInst->m_RTMask, m_pCurInst->m_MDMask, m_pCurInst->m_MDVMask, m_pCurInst->m_GLMask);
#endif

  }
  else
  {
#if defined(__GNUC__)
    sprintf(name, "Shaders/Cache/D3D10/fxerror/%s(GL%llx)@(LT%x)(RT%llx)@(MD%x)(MDV%x).cg", GetName(), m_pCurInst->m_GLMask, m_pCurInst->m_LightMask, m_pCurInst->m_RTMask, m_pCurInst->m_MDMask, m_pCurInst->m_MDVMask);
#else
    sprintf(name, "FXError/%s(GL%I64x)/(LT%x)(RT%I64x)/(MD%x)(MDV%x).cg", GetName(), m_pCurInst->m_GLMask, m_pCurInst->m_LightMask, m_pCurInst->m_RTMask, m_pCurInst->m_MDMask, m_pCurInst->m_MDVMask);
#endif
  }
#ifndef PS3
  FILE *fp = gEnv->pCryPak->FOpen(name, "w");







#endif
  if (fp)
  {
    gEnv->pCryPak->FPrintf(fp, "%s",scr);
    gEnv->pCryPak->FClose (fp);
  }
}

void CHWShader_D3D::mfOutputCompilerError(string& strErr, const char *szSrc)
{
  if (CRenderer::CV_r_shadersdebug)
  {
    FILE *fp = fxopen("$$err", "w");
    if (fp)
    {
      fputs(szSrc, fp);
      fclose (fp);
    }
  }

  string strE;
  strE.Format("FX %s shader '%s' compilation error:\n", m_eSHClass == eHWSC_Vertex ? "Vertex" : "Pixel", GetName());
  strE += strErr;
  while(true)
  {
    if (strE.find("$$in.cg") == string::npos)
      break;
    strE.replace("$$in.cg", "$$err");
  }
  OutputDebugString(strE.c_str());

	if (strE.size() >= MAX_WARNING_LENGTH)
		strE.resize(MAX_WARNING_LENGTH-1);
  Warning( "%s",strE.c_str() );
}

#ifdef WIN32
bool CHWShader_D3D::mfPostCompilingFXC(SHWSInstance *pInst, const char *szNameSrc, const char *szNameDst, LPD3DXBUFFER* ppShader, LPD3DXCONSTANTTABLE *ppConstantTable)
{
  FILE *fp = fopen(szNameDst, "rb");
  if (!fp)
  {
    //assert(0);
    remove(szNameSrc);
    return false;
  }
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (size < 20)
  {
    //assert(0);
    fclose(fp);
    remove(szNameSrc);
    remove(szNameDst);
    return false;
  }

  HRESULT hr = S_OK;
#if defined (DIRECT3D9) || defined(OPENGL)
  hr = D3DXCreateBuffer(size, ppShader);
  LPD3DXBUFFER pShader = *ppShader;
  DWORD *pBuf = (DWORD *)pShader->GetBufferPointer();
  fread(pBuf, sizeof(byte), size, fp);
  fclose(fp);

  if (!CParserBin::m_bD3D11 && !CParserBin::m_bXenon && !CParserBin::m_bPS3)
    hr = D3DXGetShaderConstantTable(pBuf, ppConstantTable);

  assert(hr == S_OK);
#elif defined (DIRECT3D10)
  D3D10CreateBlob(size, (LPD3D10BLOB *)ppShader);
  LPD3D10BLOB pShader = (LPD3D10BLOB)*ppShader;
  DWORD *pBuf = (DWORD *)pShader->GetBufferPointer();
  fread(pBuf, sizeof(byte), size, fp);
  fclose(fp);

  /*{
    ID3D11ShaderReflection *pShaderReflection;
    hr = D3D10ReflectShader(pBuf, size, &pShaderReflection);
    ID3D10Blob* pAsm = NULL;
    D3DDisassemble((UINT *)pBuf, 0, NULL, &pAsm);
    if (pAsm)
    {
      const char *szAsm = (char *)pAsm->GetBufferPointer();
      std::vector<SCGBind> InstBindVars;
      char name[256];
      mfPrepareShaderDebugInfo(&m_Insts[m_CurInst], szAsm, name, InstBindVars, (LPD3DXCONSTANTTABLE)pShaderReflection);
    }
    SAFE_RELEASE(pAsm);
    SAFE_RELEASE(pShaderReflection);
  }*/

  //PatchDXBCShaderCode(pShader, this);
  *ppShader = (LPD3DXBUFFER)pShader;
  pBuf = (DWORD *)pShader->GetBufferPointer();
  UINT nSize = pShader->GetBufferSize();

  ID3D11ShaderReflection *pShaderReflection;
	hr = D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
  if (SUCCEEDED(hr))
  {
    *ppConstantTable = (LPD3DXCONSTANTTABLE)pShaderReflection;
  }
  else
  {
    assert(0);
  }
#endif

  remove(szNameSrc);
  remove(szNameDst);

  return true;
}
#endif

SShaderAsyncInfo::~SShaderAsyncInfo()
{
  Unlink();
  if (m_pFXShader)
  {
    assert(m_pFXShader->GetID() > 0 && m_pFXShader->GetID() < MAX_REND_SHADERS);
  }
  SAFE_RELEASE(m_pFXShader);
  SAFE_RELEASE(m_pShader);
}

CryCriticalSection g_cAILock;

// Flush pended or processed shaders (main thread task)
#if defined (WIN32) || defined(XENON)
void SShaderAsyncInfo::FlushPendingShaders()
{
  if (!SShaderAsyncInfo::m_PendingList.m_Next)
  {
    SShaderAsyncInfo::m_PendingList.m_Next = &SShaderAsyncInfo::m_PendingList;
    SShaderAsyncInfo::m_PendingList.m_Prev = &SShaderAsyncInfo::m_PendingList;
    SShaderAsyncInfo::m_PendingListT.m_Next = &SShaderAsyncInfo::m_PendingListT;
    SShaderAsyncInfo::m_PendingListT.m_Prev = &SShaderAsyncInfo::m_PendingListT;
  }

  SShaderAsyncInfo *pAI, *pAINext;
  {
    AUTO_LOCK(g_cAILock);
    for (pAI=m_PendingListT.m_Next; pAI!=&m_PendingListT; pAI=pAINext)
    {
      pAINext = pAI->m_Next;
      pAI->Unlink();
      pAI->Link(&m_PendingList);
    }
  }

  for (pAI=m_PendingList.m_Next; pAI!=&m_PendingList; pAI=pAINext)
  {
    pAINext = pAI->m_Next;

    CHWShader_D3D *pSH = pAI->m_pShader;
    CHWShader_D3D::SHWSInstance *pInst = pSH->mfGetInstance(pAI->m_nOwner, pSH->m_nMaskGenShader);
#if !defined(XENON) && !defined(PS3)
    if (pAI->m_bPending == 2) // If didn't execute process yet, try to execute now
      pSH->mfCompileAsyncFXC(pInst);
    else
#endif
      pSH->mfAsyncCompileReady(pInst);
  }
}
void CShader::mfFlushPendedShaders()
{
  SShaderAsyncInfo::FlushPendingShaders();
}
#endif


#if defined (WIN32) || defined(XENON)
void CHWShader::mfFlushPendedShadersWait(int nMaxAllowed)
{
  if (nMaxAllowed>0 && SShaderAsyncInfo::s_nPendingAsyncShaders < nMaxAllowed)
    return;
  if (CRenderer::CV_r_shadersasynccompiling > 0)
  {
    iLog->Log("Flushing pended shaders...");
Start:
    while (true)
    {
      if (SShaderAsyncInfo::s_nPendingAsyncShaders <= 0)
        break;
      int n = (int)iTimer->GetAsyncCurTime();
      if (!(n % 2))
        iLog->Update();
      if (!(n % 8))
      {
        SShaderAsyncInfo::FlushPendingShaders();
      }
			else
				Sleep(1);
    }
    // Compile FXC shaders or next iteration of internal shaders
    SShaderAsyncInfo::FlushPendingShaders();

    if (SShaderAsyncInfo::s_nPendingAsyncShaders)
      goto Start;

    iLog->Log("Finished flushing pended shaders...");
  }
}

int CHWShader_D3D::mfAsyncCompileReady(SHWSInstance *pInst)
{
  //SHWSInstance *pInst = m_pCurInst;
  //assert(pInst->m_pAsync);
	if (!pInst->m_pAsync)
		return 0;

  SShaderAsyncInfo *pAsync = pInst->m_pAsync;
  int nFrame = gRenDev->GetFrameID(false);
  if (pAsync->m_nFrame == nFrame)
  {
    if (pAsync->m_fMinDistance > gRenDev->m_RP.m_fMinDistance)
      pAsync->m_fMinDistance = gRenDev->m_RP.m_fMinDistance;
  }
  else
  {
    pAsync->m_fMinDistance = gRenDev->m_RP.m_fMinDistance;
    pAsync->m_nFrame = nFrame;
  }

  DynArray<SCGBind> InstBindVars;
  ID3DXBuffer* pShader = NULL;
  LPD3DXCONSTANTTABLE pConstantTable = NULL;
  ID3DXBuffer* pErrorMsgs = NULL;
  string strErr;
  char nmDst[256], nameSrc[256];
  bool bResult;
  int nRefCount;

  SShaderTechnique *pTech = gRenDev->m_RP.m_pCurTechnique;
  CShader *pSH = pAsync->m_pFXShader;
#if !defined(XENON) && !defined(PS3)
  if (!pAsync->m_ProcessInfo.hProcess)
#endif
  {
    if (pAsync->m_bPending)
      return 0;

    mfGetDstFileName(pInst, this, nmDst, 256, 3);
    gEnv->pCryPak->AdjustFileName(nmDst, nameSrc, 0);
    if (pAsync->m_pFXShader && pAsync->m_pFXShader->m_HWTechniques.Num())
      pTech = pAsync->m_pFXShader->m_HWTechniques[0];
    if (pAsync->m_pErrors && !pAsync->m_Errors.empty())
    {
      if (CRenderer::CV_r_logShaders)
        gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: **Failed to compile 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
      string Errors = pAsync->m_Errors;
      string Text = pAsync->m_Text;
      CShader *pFXShader = pAsync->m_pFXShader;
      nRefCount = pFXShader ? pFXShader->GetRefCounter() : 0;
      nRefCount = min(nRefCount, pAsync->m_pShader ? pAsync->m_pShader->GetRefCounter() : 0);
      if (nRefCount <= 1) // Just exit if shader was deleted
      {
        pInst->m_pAsync = NULL;
        SAFE_DELETE (pAsync);
        return -1;
      }
      SAFE_DELETE (pInst->m_pAsync);
      /*if (m_Flags & HWSG_AUTOENUMTC)
      {
        gRenDev->m_RP.m_pShader = pFXShader;
        m_pCurInst = pInst;
        if (mfNextProfile(0))
        {
          bool bResult = mfActivate(HWSF_NEXT);
          if (bResult)
            return 1;
          return 0;
        }
      }*/
      {
        mfOutputCompilerError(Errors, Text.c_str());

        Warning("Couldn't compile HW shader '%s'", GetName());
        mfSaveCGFile(Text.c_str(), NULL);
      }
      return -1;
    }
    if (CRenderer::CV_r_logShaders)
      gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: Finished compiling 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
    pShader = pAsync->m_pDevShader;
    pErrorMsgs = pAsync->m_pErrors;
    pConstantTable = pAsync->m_pConstants;
    strErr = pAsync->m_Errors;
    InstBindVars = pAsync->m_InstBindVars;

    if (pAsync->m_bPendedEnv)
    {
      bResult = CHWShader_D3D::mfCreateShaderEnv(pAsync->m_nThread, pInst, pAsync->m_pDevShader, pAsync->m_pConstants, pAsync->m_pErrors, pAsync->m_InstBindVars, this, false, pAsync->m_pFXShader, pAsync->m_nCombination);
      assert(bResult == true);
    }

    // Load samplers
    if (pAsync->m_bPendedSamplers)
      mfGatherFXParameters(pInst, &pInst->m_pBindVars, &InstBindVars, this, 2, pAsync->m_pFXShader); 

    if (pAsync->m_bPendedFlush)
    {
      mfFlushCacheFile();
      strcpy(nmDst, GetName());
      char *s = strchr(nmDst, '(');
      if (s)
        s[0] = 0;
      gRenDev->m_cEF.mfInsertNewCombination(m_nMaskGenFX, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eClass, nmDst, 0);
    }

    nRefCount = pAsync->m_pFXShader ? pAsync->m_pFXShader->GetRefCounter() : 0;
    nRefCount = min(nRefCount, pAsync->m_pShader ? pAsync->m_pShader->GetRefCounter() : 0);
    if (nRefCount <= 1) // Just exit if shader was deleted
    {
      pInst->m_pAsync = NULL;
      SAFE_DELETE (pAsync);
      return -1;
    }
    SAFE_DELETE (pInst->m_pAsync);
  }
#if !defined(XENON) && !defined(PS3)
  else // if the process handle is invalid, the control will bail after WaitForSingleObject fails
  {
    int nRes = mfCompileAsyncFXC(pInst);
    if (nRes <= 0)
      return nRes;
  }
#endif
  if (pErrorMsgs && !strErr.empty())
    return -1;

  bResult = mfUploadHW(pShader, pInst, pSH, 0);
  SAFE_RELEASE(pShader);

  if (bResult)
  {
    if (pTech)
      mfGetPreprocessFlags(pTech);
    return 1;
  }
  return -1;
}

#if !defined(XENON) && !defined(PS3)
int CHWShader_D3D::mfCompileAsyncFXC(SHWSInstance *pInst)
{
  SShaderAsyncInfo *pAsync = pInst->m_pAsync;
  if (!pAsync)
    return 0;

  char nmDst[512], nameDst[512], nameSrc[512];

  if (pAsync->m_bPending)
  {
    if (SShaderAsyncInfo::s_nPendingAsyncShadersFXC >= CRenderer::CV_r_shadersasyncmaxthreads)
    {
      if (pAsync->m_bPending != 2)
      {
        pAsync->Link(&SShaderAsyncInfo::m_PendingList);
        pAsync->m_bPending = 2;
      }
      return 0;
    }
    pAsync->m_bPending = 0;
    pAsync->Link(&SShaderAsyncInfo::m_PendingList);

    mfGetDstFileName(pInst, this, nmDst, 256, 2);
    string nameUser = CRenderer::CV_r_shadersuserfolder ? gRenDev->m_cEF.m_szUserPath + string(nmDst) : string(nmDst);
    gEnv->pCryPak->AdjustFileName(nameUser.c_str(), nameDst, 0);
    if (!CRenderer::CV_r_shadersintcompiler)
    {
      string path = PathUtil::GetPath(nameDst);
      DWORD dwFileSpecAttr = GetFileAttributes(path);
      if (dwFileSpecAttr == -1)
        gEnv->pCryPak->MakeDir(path);
      else
        remove(nameDst);
    }
    mfGetDstFileName(pInst, this, nmDst, 256, 3);
    nameUser = CRenderer::CV_r_shadersuserfolder ? gRenDev->m_cEF.m_szUserPath + string(nmDst) : string(nmDst);
    gEnv->pCryPak->AdjustFileName(nameUser.c_str(), nameSrc, 0);

		char* nmDebugFlags;
		if(CRenderer::CV_r_shadersdebug==3)
			nmDebugFlags = "/Zi /Od";
		else
			nmDebugFlags = "";

    char szCmdLine[1024];
#if defined (DIRECT3D9)
    if (CParserBin::m_bD3D11)
      sprintf(szCmdLine, "fxc.exe /nologo /T %s /Zpr /Gec %s /Fh %s %s", nmDebugFlags, pAsync->m_Profile.c_str(), nameDst, nameSrc);
    else
    if (CParserBin::m_bXenon)
      sprintf(szCmdLine, "fxcx.exe /nologo /T %s /Zpr /Fo %s %s", pAsync->m_Profile.c_str(), nameDst, nameSrc);
    else
    if (CParserBin::m_bPS3)
      sprintf(szCmdLine, "DXPSShaderCompiler.exe %s %s %s", pAsync->m_Profile.c_str(), nameDst, nameSrc);
    else
      sprintf(szCmdLine, "fxc.exe /nologo /T %s /Zpr /Gec %s /Fo %s %s", nmDebugFlags, pAsync->m_Profile.c_str(), nameDst, nameSrc);
#elif defined (DIRECT3D10)
    sprintf(szCmdLine, "fxc.exe /Gec /nologo /T %s /Zpr %s /Fo \"%s\" \"%s\"", nmDebugFlags, pAsync->m_Profile.c_str(), nameDst, nameSrc);
#endif
    strcat(szCmdLine, " /E ");
    strcat(szCmdLine, pAsync->m_Name.c_str());

    // make command for execution
    FILE *fp = fopen(nameSrc, "w");
    if (!fp)
      return 0;
    fputs(pAsync->m_Text.c_str(), fp);
    fclose (fp);

	  // create pipe
	  SECURITY_ATTRIBUTES sa;
	  ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	  sa.bInheritHandle = TRUE;
	  sa.lpSecurityDescriptor = 0;

	  if (!CreatePipe(&pInst->m_pAsync->m_hPipeOutputRead, &pInst->m_pAsync->m_hPipeOutputWrite, &sa, 0))
	  {
		  SAFE_DELETE (pInst->m_pAsync);
		  iLog->LogError("CreatePipe failed! Needed for out of process shader compilation.");
		  return 0;
	  }

	  // create process
	  STARTUPINFO si;
	  ZeroMemory(&si, sizeof(STARTUPINFO));
	  si.cb = sizeof(STARTUPINFO);
	  si.hStdOutput = pInst->m_pAsync->m_hPipeOutputWrite;
	  si.hStdError   = pInst->m_pAsync->m_hPipeOutputWrite;
	  si.dwFlags |= STARTF_USESTDHANDLES;

	  ZeroMemory(&pInst->m_pAsync->m_ProcessInfo, sizeof(PROCESS_INFORMATION));

	  if( !CreateProcess( NULL, // No module name (use command line). 
		  szCmdLine,        // Command line. 
		  NULL,             // Process handle not inheritable. 
		  NULL,             // Thread handle not inheritable. 
		  TRUE,             // Set handle inheritance to TRUE. 
		  CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, // No creation flags. 
		  NULL,             // Use parent's environment block. 
		  NULL,             // Set starting directory. 
		  &si,              // Pointer to STARTUPINFO structure.
		  &pInst->m_pAsync->m_ProcessInfo )             // Pointer to PROCESS_INFORMATION structure.
		  ) 
	  {
		  CloseHandle(pInst->m_pAsync->m_hPipeOutputRead);
		  CloseHandle(pInst->m_pAsync->m_hPipeOutputWrite);
		  SAFE_DELETE (pInst->m_pAsync);
		  iLog->LogError("CreateProcess failed: %s", szCmdLine);
		  return 0;
	  }
    int i;
    for (i=0; i<gcpRendD3D->m_AsyncShaderTasks.size(); i++)
    {
      if (gcpRendD3D->m_AsyncShaderTasks[i]->GetThreadFXC() == -1)
        break;
    }
    pAsync->m_nThread = i;
    gcpRendD3D->m_AsyncShaderTasks[i]->SetThreadFXC(1);
    SShaderAsyncInfo::s_nPendingAsyncShadersFXC++;
    //iLog->Log("Start (%d): %s(%d)", pAsync->m_nThread, m_EntryFunc.c_str(), pInst->m_RTMask);
  }
  else
  {
		// TODO: The following case picks up results from shader compiling in case r_ShadersIntCompiler = 0 / CV_r_ShadersAsyncCompiling = 1
		// Might have to wait for the output pipe here as well as in case r_ShadersIntCompiler = 0 / CV_r_ShadersAsyncCompiling = 0 to prevent contention
		// See CHWShader_D3D::mfCompileHLSL_Int() for reference
    if (WaitForSingleObject (pAsync->m_ProcessInfo.hProcess, 0) != WAIT_OBJECT_0)
      return 0;

    SShaderAsyncInfo::s_nPendingAsyncShadersFXC--;
    assert(pAsync->m_nThread < gcpRendD3D->m_AsyncShaderTasks.size());
    if (pAsync->m_nThread < gcpRendD3D->m_AsyncShaderTasks.size())
      gcpRendD3D->m_AsyncShaderTasks[pAsync->m_nThread]->SetThreadFXC(-1);

    int nRefCount;

    pAsync->Unlink();
    mfGetDstFileName(pInst, this, nmDst, 256, 2);
    gEnv->pCryPak->AdjustFileName(nmDst, nameDst, 0);
    mfGetDstFileName(pInst, this, nmDst, 256, 3);
    gEnv->pCryPak->AdjustFileName(nmDst, nameSrc, 0);
    FILE *fp = gEnv->pCryPak->FOpen(nameDst, "rb");
    if (!fp)
    {
      if (CRenderer::CV_r_logShaders)
        gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: **Failed to compile 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
			CloseHandle(pAsync->m_hPipeOutputRead);
			CloseHandle(pAsync->m_hPipeOutputWrite);
      CloseHandle(pAsync->m_ProcessInfo.hProcess);
      CloseHandle(pAsync->m_ProcessInfo.hThread);
      nRefCount = pAsync->m_pFXShader ? pAsync->m_pFXShader->GetRefCounter() : 0;
      nRefCount = min(nRefCount, pAsync->m_pShader ? pAsync->m_pShader->GetRefCounter() : 0);
      if (nRefCount <= 1) // Just exit if shader was deleted
      {
        pInst->m_pAsync = NULL;
        SAFE_DELETE (pAsync);
        return -1;
      }
      SAFE_DELETE (pInst->m_pAsync);
      remove(nameSrc);
      /*if (m_Flags & HWSG_AUTOENUMTC)
      {
        m_pCurInst = pInst;
        if (mfNextProfile(0))
        {
          bool bResult = mfActivate(HWSF_NEXT);
          if (bResult)
            return 1;
          return 0;
        }
      }*/
      return -1;
    }
    if (CRenderer::CV_r_logShaders)
      gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: Finished compiling 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
    gEnv->pCryPak->FClose(fp);
    bool bResult = mfPostCompilingFXC(pInst, nameSrc, nameDst, &pAsync->m_pDevShader, &pAsync->m_pConstants);
    bResult &= mfCreateShaderEnv(pAsync->m_nThread, pInst, pAsync->m_pDevShader, pAsync->m_pConstants, pAsync->m_pErrors, pAsync->m_InstBindVars, this, true, pAsync->m_pFXShader, pAsync->m_nCombination);

		CloseHandle(pAsync->m_hPipeOutputRead);
		CloseHandle(pAsync->m_hPipeOutputWrite);
		CloseHandle(pAsync->m_ProcessInfo.hProcess);
		CloseHandle(pAsync->m_ProcessInfo.hThread);

    //iLog->Log("Finish (%d): %s(%d)", pAsync->m_nThread, m_EntryFunc.c_str(), pInst->m_RTMask);

    nRefCount = pAsync->m_pFXShader ? pAsync->m_pFXShader->GetRefCounter() : 0;
    nRefCount = min(nRefCount, pAsync->m_pShader ? pAsync->m_pShader->GetRefCounter() : 0);
    mfUploadHW(pAsync->m_pDevShader, pInst, pAsync->m_pFXShader, 0);
    if (nRefCount <= 1) // Just exit if shader was deleted
    {
      SAFE_RELEASE(pAsync->m_pDevShader);
      pInst->m_pAsync = NULL;
      SAFE_DELETE (pAsync);
      return -1;
    }
    SAFE_RELEASE(pAsync->m_pDevShader);
    SAFE_DELETE (pInst->m_pAsync);
  }
  return 1;
}
#endif // XENON

//DynArray<CHWShader_D3D::SHWSInstance> *pInstCont0;
//DynArray<CHWShader_D3D::SHWSInstance> *pInstCont1;

bool CHWShader_D3D::mfRequestAsync(SHWSInstance *pInst, DynArray<SCGBind>& InstBindVars, const char *prog_text, const char *szProfile, const char *szEntry)
{
#ifdef SHADER_ASYNC_COMPILATION
  char nameSrc[256], nmDst[256];
  mfGetDstFileName(pInst, this, nmDst, 256, 3);
  gEnv->pCryPak->AdjustFileName(nmDst, nameSrc, 0);

  if (!SShaderAsyncInfo::m_PendingList.m_Next)
  {
    SShaderAsyncInfo::m_PendingList.m_Next = &SShaderAsyncInfo::m_PendingList;
    SShaderAsyncInfo::m_PendingList.m_Prev = &SShaderAsyncInfo::m_PendingList;
    SShaderAsyncInfo::m_PendingListT.m_Next = &SShaderAsyncInfo::m_PendingListT;
    SShaderAsyncInfo::m_PendingListT.m_Prev = &SShaderAsyncInfo::m_PendingListT;
  }

  if (!m_pCache || !m_pCache->m_pRes[CACHE_USER])
  {
    if (m_pCache)
      m_pCache->m_nRefCount--;
    m_pCache = mfInitCache(NULL, this, true, m_CRC32, false, false);
  }

  pInst->m_pAsync = new SShaderAsyncInfo;
  pInst->m_pAsync->m_fMinDistance = gRenDev->m_RP.m_fMinDistance;
  pInst->m_pAsync->m_nFrame = gRenDev->GetFrameID(false);
  pInst->m_pAsync->m_InstBindVars = InstBindVars;
  pInst->m_pAsync->m_pShader = this;
  pInst->m_pAsync->m_pShader->AddRef();
  pInst->m_pAsync->m_pFXShader = gRenDev->m_RP.m_pShader;
  pInst->m_pAsync->m_pFXShader->AddRef();
  pInst->m_pAsync->m_nCombination = gRenDev->m_cEF.m_nCombinationsProcess;
  assert(!stricmp(m_NameSourceFX.c_str(), pInst->m_pAsync->m_pFXShader->m_NameFile.c_str()));
  if (m_Flags & HWSG_SHARED)
  {
    assert(m_nMaskGenShader == pInst->m_GLMask);
    DynArray<SHWSInstance> *pInstCont = mfGetSharedInstContainer(false, m_nMaskGenShader, false);
    assert(pInstCont);
    pInst->m_pAsync->m_nOwner = (int)(pInst - &(*pInstCont)[0]);
  }
  else
    pInst->m_pAsync->m_nOwner = (int)(pInst - &m_Insts[0]);
  pInst->m_pAsync->m_RTMask = pInst->m_RTMask;
  pInst->m_pAsync->m_LightMask = pInst->m_LightMask;
  pInst->m_pAsync->m_MDMask = pInst->m_MDMask;
  pInst->m_pAsync->m_MDVMask = pInst->m_MDVMask;
  pInst->m_pAsync->m_eClass = pInst->m_eClass;

  /*if (!stricmp(szEntry, "Common_ShadowPS") && pInst->m_pAsync->m_nOwner == 2)
  {
    pInstCont0 = mfGetSharedInstContainer(false, m_nMaskGenShader, false);
    int nnn = 0;
  }
  if (!stricmp(szEntry, "Common_ShadowVS") && pInst->m_pAsync->m_nOwner == 2)
  {
    pInstCont1 = mfGetSharedInstContainer(false, m_nMaskGenShader, false);
    int nnn = 0;
  }*/
  pInst->m_pAsync->m_Text = prog_text;
  pInst->m_pAsync->m_Name = szEntry;
  pInst->m_pAsync->m_Profile = szProfile;

#if !defined(XENON) && !defined(PS3)
  if (!CRenderer::CV_r_shadersintcompiler)
    mfCompileAsyncFXC(pInst);
  else
#endif
    CAsyncShaderTask::InsertPendingShader(pInst->m_pAsync);

  if (CRenderer::CV_r_logShaders)
    gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async %d: Requested compiling 0x%x '%s' shader\n", gRenDev->GetFrameID(false), pInst, nameSrc);
#endif
  return false;
}

#else // WIN32
void CHWShader::mfFlushPendedShadersWait(int nMaxAllowed)
{
}
#endif

bool CHWShader_D3D::mfCompileHLSL_Int(char *prog_text, LPD3DXBUFFER* ppShader, LPD3DXCONSTANTTABLE *ppConstantTable, LPD3DXBUFFER* ppErrorMsgs, string& strErr, DynArray<SCGBind>& InstBindVars)
{
  HRESULT hr = S_OK;
  SHWSInstance *pInst = m_pCurInst;
  const char *szProfile = mfProfileString(pInst->m_eClass);
  const char *pFunCCryName = m_EntryFunc.c_str();

  bool bRes = true; 
  if (CRenderer::CV_r_shadersdebug == 2)
  {



    mfSaveCGFile(prog_text, "TestCG");

  }
#if defined (XENON) || defined (WIN32)
  if (CRenderer::CV_r_shadersasynccompiling && !(m_Flags & HWSG_SYNC))
  {
    return mfRequestAsync(pInst, InstBindVars, prog_text, szProfile, pFunCCryName);
  }
  else
#endif
	if(CRenderer::CV_r_shadersremotecompiler)
	{
		const char* pCompiler = gRenDev->m_cEF.mfGetShaderCompileFlags(pInst->m_eClass);

		//////////////////////////////////////////////////////////////////////////
		// Generate request line text.
		char szShaderGenName[512];
		strcpy_s(szShaderGenName, GetName());
		char *s = strchr(szShaderGenName, '(');
		if (s) s[0] = 0;
    string RequestLine;
		gRenDev->m_cEF.mfInsertNewCombination(m_nMaskGenFX, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eClass, szShaderGenName, 0, &RequestLine, false);
		//////////////////////////////////////////////////////////////////////////

		std::vector<uint8> Data;
		if(!NRemoteCompiler::CShaderSrv::Instance().Compile(Data,szProfile,prog_text,pFunCCryName,pCompiler,RequestLine.c_str()))
		{
			string sErrorText;
			sErrorText.reserve(Data.size());
			for (uint32 i = 0; i < Data.size(); i++)
				sErrorText += Data[i];
			strErr = sErrorText;

			return false;
		}

#if defined (DIRECT3D9) || defined(OPENGL)
		hr = D3DXCreateBuffer(Data.size(), ppShader);
		LPD3DXBUFFER pShader = *ppShader;
		DWORD *pBuf = (DWORD *)pShader->GetBufferPointer();
		memcpy(pBuf,&Data[0],Data.size());

		if (!CParserBin::m_bPS3 && !CParserBin::m_bD3D11)
			hr = D3DXGetShaderConstantTable(pBuf, ppConstantTable);

		assert(hr == S_OK);
#elif defined (DIRECT3D10)
		D3D10CreateBlob(Data.size(), (LPD3D10BLOB *)ppShader);
		LPD3D10BLOB pShader = (LPD3D10BLOB)*ppShader;
		DWORD *pBuf = (DWORD *)pShader->GetBufferPointer();
		memcpy(pBuf,&Data[0],Data.size());

		*ppShader = (LPD3DXBUFFER)pShader;
		pBuf = (DWORD *)pShader->GetBufferPointer();
		UINT nSize = pShader->GetBufferSize();

		ID3D11ShaderReflection *pShaderReflection;
		hr = D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
		if (SUCCEEDED(hr))
		{
			*ppConstantTable = (LPD3DXCONSTANTTABLE)pShaderReflection;
		}
		else
		{
			assert(0);
		}
#endif

		return hr == S_OK;
	}
	else
  if (CRenderer::CV_r_shadersintcompiler && !CParserBin::m_bD3D11)
  {
#if defined (DIRECT3D9) || defined(OPENGL)
    int nFlags = D3DXSHADER_PACKMATRIX_ROWMAJOR; // | D3DXSHADER_AVOID_FLOW_CONTROL;// |  D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;





    nFlags |= D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY;
    //if( pInst->m_eProfileType == eHWSP_VS_2_0 )
    //  nFlags |= D3DXSHADER_AVOID_FLOW_CONTROL;


		if (CRenderer::CV_r_shadersdebug == 3)
			nFlags |= D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;

    hr = D3DXCompileShader(prog_text, strlen(prog_text), NULL, NULL, pFunCCryName, szProfile, nFlags, ppShader, ppErrorMsgs, ppConstantTable); 
    if (FAILED(hr))
    {
      if (*ppErrorMsgs)
      {
        const char *err = (const char *)ppErrorMsgs[0]->GetBufferPointer();
        strErr += err;
      }
      else
      { 
        strErr += "D3DXCompileShader failed";
      }
      bRes = false;
    }
    return bRes;
#elif defined (DIRECT3D10)
		uint32 nFlags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
		if (CRenderer::CV_r_shadersdebug == 3)
			nFlags |= D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;

    hr = D3DX11CompileFromMemory(prog_text,
      strlen(prog_text),
      GetName(),
      NULL,
      NULL,
      pFunCCryName,
      szProfile,
      nFlags,
      0,
      NULL,
      (ID3D10Blob **)ppShader,
      (ID3D10Blob **)ppErrorMsgs, &hr);
    if (FAILED(hr) || !*ppShader)
    {
      if (*ppErrorMsgs)
      {
        const char *err = (const char *)ppErrorMsgs[0]->GetBufferPointer();
        strErr += err;
      }
      else
      {
        strErr += "D3DXCompileShader failed";
      }
      bRes = false;
    }
    else
    {
      ID3D11ShaderReflection *pShaderReflection;
      UINT *pData = (UINT *)ppShader[0]->GetBufferPointer();
      UINT nSize = ppShader[0]->GetBufferSize();
			hr = D3DReflect(pData, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
      if (SUCCEEDED(hr))
      {
        *ppConstantTable = (LPD3DXCONSTANTTABLE)pShaderReflection;
      }
      else
      {
        assert(0);
      }
    }
    return bRes;
#endif
  }
#if defined(WIN32) && !defined (OPENGL)
  else
  {
    char nmDst[256], nameDst[512], nameSrc[512];
    mfGetDstFileName(pInst, this, nmDst, 256, 2);
    string nameUser = CRenderer::CV_r_shadersuserfolder ? gRenDev->m_cEF.m_szUserPath + string(nmDst) : string(nmDst);
    gEnv->pCryPak->AdjustFileName(nameUser.c_str(), nameDst, 0);
    if (!CRenderer::CV_r_shadersintcompiler || CParserBin::m_bD3D11)
    {
      string path = PathUtil::GetPath(nameDst);
      DWORD dwFileSpecAttr = GetFileAttributes(path);
      if (dwFileSpecAttr == -1)
        gEnv->pCryPak->MakeDir(path);
      else
        remove(nameDst);
    }
    mfGetDstFileName(pInst, this, nmDst, 256, 3);
    nameUser = CRenderer::CV_r_shadersuserfolder ? gRenDev->m_cEF.m_szUserPath + string(nmDst) : string(nmDst);
    gEnv->pCryPak->AdjustFileName(nameUser.c_str(), nameSrc, 0);

    char szCmdLine[1024];
#if defined (DIRECT3D9)
    if (CParserBin::m_bD3D11)
      sprintf(szCmdLine, "fxc.exe /nologo /T %s /Zpr /Gec /Fh %s %s", szProfile, nameDst, nameSrc);
    else
    if (CParserBin::m_bXenon)
      sprintf(szCmdLine, "fxcx.exe /nologo /T %s /Zpr /Fo %s %s", szProfile, nameDst, nameSrc);
    else
    if (CParserBin::m_bPS3) //temporal workaround, all calls will be anyway redirected to the server later on
      sprintf(szCmdLine, "DXPSShaderCompiler.exe %s %s %s", szProfile, nameDst, nameSrc);
    else
      sprintf(szCmdLine, "fxc.exe /nologo /T %s /Zpr /Gec /Fo %s %s", szProfile, nameDst, nameSrc);
#elif defined (DIRECT3D10)
    sprintf(szCmdLine, "fxc.exe /Gec /nologo /T %s /Zpr /Fo \"%s\" \"%s\"", szProfile, nameDst, nameSrc);
#endif
    strcat(szCmdLine, " /E ");
    strcat(szCmdLine, pFunCCryName);

    // make command for execution
    FILE *fp = fopen(nameSrc, "w");
    if (!fp)
      return NULL;
    fputs(prog_text, fp);
    fclose (fp);

    pInst->m_pAsync = new SShaderAsyncInfo;
    pInst->m_pAsync->m_bPending = false;
    pInst->m_pAsync->m_InstBindVars = InstBindVars;
    pInst->m_pAsync->m_pShader = this;
    pInst->m_pAsync->m_pShader->AddRef();
    pInst->m_pAsync->m_pFXShader = gRenDev->m_RP.m_pShader;
    pInst->m_pAsync->m_pFXShader->AddRef();
    pInst->m_pAsync->m_nCombination = gRenDev->m_cEF.m_nCombinationsProcess;
    if (m_Flags & HWSG_SHARED)
    {
      assert(m_nMaskGenShader == pInst->m_GLMask);
      DynArray<SHWSInstance> *pInstCont = mfGetSharedInstContainer(false, m_nMaskGenShader, false);
      assert(pInstCont);
      pInst->m_pAsync->m_nOwner = (int)(pInst - &(*pInstCont)[0]);
    }
    else
      pInst->m_pAsync->m_nOwner = (int)(pInst - &m_Insts[0]);
    pInst->m_pAsync->m_RTMask = pInst->m_RTMask;
    pInst->m_pAsync->m_LightMask = pInst->m_LightMask;
    pInst->m_pAsync->m_MDMask = pInst->m_MDMask;
    pInst->m_pAsync->m_MDVMask = pInst->m_MDVMask;
    pInst->m_pAsync->m_eClass = pInst->m_eClass;

		// create pipe
		SECURITY_ATTRIBUTES sa;
		ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = 0;

		if (!CreatePipe(&pInst->m_pAsync->m_hPipeOutputRead, &pInst->m_pAsync->m_hPipeOutputWrite, &sa, 0))
		{
			SAFE_DELETE (pInst->m_pAsync);
			iLog->LogError("CreatePipe failed! Needed for out of process shader compilation.");
			return false;
		}

		// create process
		STARTUPINFO si;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.hStdOutput = pInst->m_pAsync->m_hPipeOutputWrite;
		si.hStdError   = pInst->m_pAsync->m_hPipeOutputWrite;
		si.dwFlags |= STARTF_USESTDHANDLES;

		ZeroMemory(&pInst->m_pAsync->m_ProcessInfo, sizeof(PROCESS_INFORMATION));

		if( !CreateProcess( NULL, // No module name (use command line). 
			szCmdLine,        // Command line. 
			NULL,             // Process handle not inheritable. 
			NULL,             // Thread handle not inheritable. 
			TRUE,             // Set handle inheritance to TRUE. 
			CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, // No creation flags. 
			NULL,             // Use parent's environment block. 
			NULL,             // Set starting directory. 
			&si,              // Pointer to STARTUPINFO structure.
			&pInst->m_pAsync->m_ProcessInfo )             // Pointer to PROCESS_INFORMATION structure.
			) 
		{
			CloseHandle(pInst->m_pAsync->m_hPipeOutputRead);
			CloseHandle(pInst->m_pAsync->m_hPipeOutputWrite);
			SAFE_DELETE (pInst->m_pAsync);
			iLog->LogError("CreateProcess failed: %s", szCmdLine);
			return false;
		}

		DWORD waitResult = 0;
		HANDLE waitHandles[] = { pInst->m_pAsync->m_ProcessInfo.hProcess, pInst->m_pAsync->m_hPipeOutputRead };
		while(true)
		{
			waitResult = WaitForMultipleObjects(sizeof(waitHandles) / sizeof(waitHandles[0]), waitHandles, FALSE, 60000L);
			if (waitResult == WAIT_FAILED)
				break;

			DWORD bytesRead, bytesAvailable;
			while(PeekNamedPipe(pInst->m_pAsync->m_hPipeOutputRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable)
			{
				char buff[4096];
				ReadFile(pInst->m_pAsync->m_hPipeOutputRead, buff, sizeof(buff)-1, &bytesRead, 0);
				buff[bytesRead] = '\0';
				strErr += buff;
			}

			if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT)
				break;
		}

		CloseHandle(pInst->m_pAsync->m_hPipeOutputRead);
		CloseHandle(pInst->m_pAsync->m_hPipeOutputWrite);

		if (waitResult == WAIT_TIMEOUT)
		{
			iLog->LogWarning ("fxc takes forever to compile shader. Timeout of 60 seconds reached... terminating process!");
			TerminateProcess(pInst->m_pAsync->m_ProcessInfo.hProcess, 0);
		}

		bRes = mfPostCompilingFXC(pInst, nameSrc, nameDst, ppShader, ppConstantTable);

    if (CRenderer::CV_r_logShaders > 1)
    {
      if (bRes)
        gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Sync: Succeeded compiling '%s' shader\n", nameSrc);
      else
        gcpRendD3D->LogShv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Sync: Failed compiling '%s' shader\n", nameSrc);
    }

    CloseHandle(pInst->m_pAsync->m_ProcessInfo.hProcess);
    CloseHandle(pInst->m_pAsync->m_ProcessInfo.hThread);

    SAFE_DELETE (pInst->m_pAsync);

    return bRes;
  }
#endif // Win32

  return false;
}

/*bool CHWShader_D3D::mfNextProfile(uint32 nFlags)
{
  CD3D9Renderer *rd = gcpRendD3D;
  SHWSInstance *pInst = m_pCurInst;
  if (!(rd->GetFeatures() & RFT_HW_PS20))
    return false;
  switch (pInst->m_eProfileType)
  {
  case eHWSP_PS_2_X:
  case eHWSP_VS_2_0:
  case eHWSP_PS_3_0:
  case eHWSP_PS_4_0:
  case eHWSP_GS_4_0:
  case eHWSP_VS_4_0:
  case eHWSP_VS_3_0:
    return false;
  case eHWSP_VS_1_1:
    pInst->m_eProfileType = eHWSP_VS_2_0;
    break;

  case eHWSP_PS_1_1:
    pInst->m_eProfileType = eHWSP_PS_2_0;
    break;
  case eHWSP_PS_2_0:
    if (rd->m_Features & RFT_HW_PS2X)
      pInst->m_eProfileType = eHWSP_PS_2_X;
    else
      return false;
    break;
  }

  return true;
}*/
LPD3DXBUFFER CHWShader_D3D::mfCompileHLSL(char *prog_text, LPD3DXCONSTANTTABLE *ppConstantTable, LPD3DXBUFFER* ppErrorMsgs, uint32 nFlags, DynArray<SCGBind>& InstBindVars)
{
//	LOADING_TIME_PROFILE_SECTION(iSystem);

  // Test adding source text to context
  SHWSInstance *pInst = m_pCurInst;
  string strErr;
  LPD3DXBUFFER pCode = NULL;
  HRESULT hr = S_OK;
  if (!prog_text)
  {
    assert(0);
    return NULL;
  }
  if (CRenderer::CV_r_shadersnocompile)
    return NULL;

  bool bResult = mfCompileHLSL_Int(prog_text, &pCode, ppConstantTable, ppErrorMsgs, strErr, InstBindVars);
  if (!pCode)
  {
    if (CRenderer::CV_r_shadersasynccompiling)
      return NULL;
    /*if (m_Flags & HWSG_AUTOENUMTC)
    {
      while (mfNextProfile(nFlags))
      {
        bResult = mfCompileHLSL_Int(prog_text, &pCode, ppConstantTable, ppErrorMsgs, strErr, InstBindVars);
        if (pCode)
          break;
        if (pInst->IsAsyncCompiling())
          return NULL;
      }
    }*/
    if (!pCode)
    {
      //int nLights = pInst->m_LightMask & 0xf;
      //if (nLights > 1)
      //  iLog->Log("WARNING: Shader '%s' was failed to compile for %d light sources (fallback to %d light sources)", GetName(), nLights, nLights-1);
      //else
      {
        mfOutputCompilerError(strErr, prog_text);

        Warning("Couldn't compile HW shader '%s'", GetName());
        mfSaveCGFile(prog_text, NULL);
      }
    }
  }

  return pCode;
}

DynArray<CHWShader_D3D::SHWSInstance> *CHWShader_D3D::mfGetSharedInstContainer(bool bCreate, uint64 GLMask, bool bPrecache)
{
  DynArray<SHWSInstance> *pInstCont;
  InstanceMapItor itInst = m_SharedInsts.find(m_EntryFunc);
  SHWSSharedList *pInstSH = NULL;
  if (itInst == m_SharedInsts.end())
  {
    if (!bCreate)
      return NULL;
    pInstSH = new SHWSSharedList;
    m_SharedInsts.insert(InstanceMapItor::value_type(m_EntryFunc, pInstSH));
  }
  else
    pInstSH = itInst->second;
  int i;
  SHWSSharedInstance *pSHI = NULL;
  for (i=0; i<pInstSH->m_SharedInsts.size(); i++)
  {
    pSHI = &pInstSH->m_SharedInsts[i];
    if (pSHI->m_GLMask == GLMask)
      break;
  }
  if (i == pInstSH->m_SharedInsts.size())
  {
    if (!bCreate)
      return NULL;
    SHWSSharedInstance SI;
    SI.m_GLMask = GLMask;
    pInstSH->m_SharedInsts.push_back(SI);
    pSHI = &pInstSH->m_SharedInsts[i];
  }
	assert(pSHI);
  if (bPrecache || gRenDev->m_bEditor)
  {
		if ( gRenDev->m_RP.m_pShader )
		{
			const char *nm = gRenDev->m_RP.m_pShader->m_NameShader.c_str();
			for (i=0; i<pInstSH->m_SharedNames.size(); i++)
			{
				SHWSSharedName *pSHN = &pInstSH->m_SharedNames[i];
				if (!stricmp(pSHN->m_Name.c_str(), nm))
					break;
			}
			if (i == pInstSH->m_SharedNames.size())
			{
				SHWSSharedName NM;
				NM.m_Name = nm;
				NM.m_CRC32 = m_CRC32;
				pInstSH->m_SharedNames.push_back(NM);
			}
		}
  }
  pInstCont = &pSHI->m_Insts;

  return pInstCont;
}

void CHWShader_D3D::mfPrepareShaderDebugInfo(SHWSInstance *pInst, CHWShader_D3D *pSH, const char *szAsm, DynArray<SCGBind>& InstBindVars, LPD3DXCONSTANTTABLE pConstantTable)
{
#ifndef XENON
  if (szAsm)
  {
    char *szInst = strstr((char *)szAsm, "pproximately ");
    if (szInst)
      pInst->m_nInstructions = atoi(&szInst[13]);
  }
#endif
  if (CRenderer::CV_r_shadersdebug)
  {
    char nmdst[256];
    mfGetDstFileName(pInst, pSH, nmdst, 256, 4);








#ifndef PS3
    string szName = CRenderer::CV_r_shadersuserfolder ? gRenDev->m_cEF.m_szUserPath + string(nmdst) + string(".fxca") : string(nmdst) + string(".fxca");
    FILE *statusdst = gEnv->pCryPak->FOpen(szName.c_str(), "wb");





#endif

    if (statusdst)
    {
      gEnv->pCryPak->FPrintf(statusdst, "\n// %s %s\n\n", "%STARTSHADER", mfProfileString(pInst->m_eClass));
      if (pSH->m_eSHClass == eHWSC_Vertex)
      {
        for (uint32 i=0; i<(uint32)InstBindVars.size(); i++)
        {
          SCGBind *pBind = &InstBindVars[i];
          gEnv->pCryPak->FPrintf(statusdst, "//   %s %s %d %d\n", "%%", pBind->m_Name.c_str(), pBind->m_nParameters, pBind->m_dwBind);
        }
      }
      gEnv->pCryPak->FPrintf(statusdst, "%s", szAsm);
      gEnv->pCryPak->FPrintf(statusdst, "\n// %s\n", "%ENDSHADER");
      gEnv->pCryPak->FClose(statusdst);
    }
    pInst->m_Handle.m_pShader = NULL;
  }
}

void CHWShader_D3D::mfPrintCompileInfo(SHWSInstance *pInst)
{
  int nConsts = 0;
  int nParams = pInst->m_pBindVars.size();
  for (int i=0; i<nParams; i++)
  {
    SCGBind *pB = &pInst->m_pBindVars[i];
    nConsts += pB->m_nParameters;
  }

  char szGenName[512];
  strncpy(szGenName, GetName(), sizeof(szGenName)-1);
	szGenName[sizeof(szGenName)-1] = '\0';
  char *s = strchr(szGenName, '(');
  if (s)
    s[0] = 0;
  if (CRenderer::CV_r_shadersdebug == 2)
  {
    string pName;
    gRenDev->m_cEF.mfInsertNewCombination(m_nMaskGenFX, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eClass, szGenName, 0, &pName, false);
    CryLog(" Compile %s (%d instructions, %d/%d constants) ... ", pName.c_str(), pInst->m_nInstructions, nParams, nConsts);
    int nSize = strlen(szGenName);
    mfGenName(pInst, &szGenName[nSize], 512, 1);
    CryLog("           --- Cache entry: %s", szGenName);
  }
  else
  {
    int nSize = strlen(szGenName);
    mfGenName(pInst, &szGenName[nSize], 512, 1);
    CryLog(" Compile %s (%d instructions, %d/%d constants) ... ", szGenName, pInst->m_nInstructions, nParams, nConsts);
  }

  if (gRenDev->m_cEF.m_bActivated && CRenderer::CV_r_shadersdebug > 0)
    CryLog(
    " Shader %s"
#if defined(__GNUC__)
    "(%llx)"
#else
    "(%I64x)"
#endif
    "(%x)(%x)(%x)(%s) wasn't compiled before preactivating phase",
    GetName(), pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask,  mfProfileString(pInst->m_eClass));
}

bool CHWShader_D3D::mfCreateShaderEnv(int nThread, SHWSInstance *pInst, LPD3DXBUFFER pShader, LPD3DXCONSTANTTABLE pConstantTable, LPD3DXBUFFER pErrorMsgs, DynArray<SCGBind>& InstBindVars, CHWShader_D3D *pSH, bool bShaderThread, CShader *pFXShader, int nCombination, const char *src)
{
  // Create asm (.fxca) cache file
  assert(pInst);
  if (!pInst)
    return false;

  CSpinLock lock;

  if (pInst->m_pBindVars.size())
    return true;

  if (pShader && (nCombination < 0))
  {
#if defined(DIRECT3D9) || defined(OPENGL)
    if (!CParserBin::m_bD3D11)
    {
      LPD3DXBUFFER pAsm = NULL;































      D3DXDisassembleShader((DWORD *)pShader->GetBufferPointer(), FALSE, NULL, &pAsm);

      if (pAsm)
      {
        char *szAsm = (char *)pAsm->GetBufferPointer();
        mfPrepareShaderDebugInfo(pInst, pSH, szAsm, InstBindVars, pConstantTable);
      }
      SAFE_RELEASE(pAsm);






    }
#elif defined(DIRECT3D10)
    ID3D10Blob* pAsm = NULL;
    ID3D10Blob* pSrc = (ID3D10Blob *)pShader;
    UINT *pBuf = (UINT *)pSrc->GetBufferPointer();
    D3DDisassemble(pBuf, pSrc->GetBufferSize(), 0, NULL, &pAsm);
    if (pAsm)
    {
      char *szAsm = (char *)pAsm->GetBufferPointer();
      mfPrepareShaderDebugInfo(pInst, pSH, szAsm, InstBindVars, pConstantTable);
    }
    SAFE_RELEASE(pAsm);
#endif
  }
  //assert(!pInst->m_pBindVars);

  if (pShader)
  {
    bool bVF = pSH->m_eSHClass == eHWSC_Vertex;
#if !defined(XENON) && !defined(PS3)
    if (CParserBin::m_bXenon || CParserBin::m_bPS3)
      bVF = false;
#endif
    if (bVF)
      mfVertexFormat(pInst, pSH, pShader, InstBindVars);
    {
#ifdef OPENGL
      if (pInst->m_Handle.IsValid() == ED3DShError_Ok)
      {
        SAFE_RELEASE(pConstantTable);
        D3DXBuildConstantsTable((IDirect3DBaseShader9*)pInst->m_Handle.m_pHandle, &pConstantTable);
      }
#endif
    }
		if (pConstantTable)
			mfCreateBinds(pInst, pConstantTable, (byte *)pShader->GetBufferPointer(), pShader->GetBufferSize());
  }
  if (!(pSH->m_Flags & HWSG_PRECACHEPHASE))
  {
    int nConsts = 0;
    int nParams = pInst->m_pBindVars.size();
    for (int i=0; i<nParams; i++)
    {
      SCGBind *pB = &pInst->m_pBindVars[i];
      nConsts += pB->m_nParameters;
    }
    if (gRenDev->m_cEF.m_nCombinationsProcess >= 0)
    {
      //assert(!bShaderThread);

      //if (!(gRenDev->m_cEF.m_nCombination & 0xff))              
      if (!CParserBin::m_bD3D11 && !CParserBin::m_bXenon && !CParserBin::m_bPS3)
      {
        CryLog("%d: Compile %s %s (%d out of %d) - (%d/%d constants) ... ", nThread,
          mfProfileString(pInst->m_eClass), pSH->GetName(), nCombination, gRenDev->m_cEF.m_nCombinationsProcessOverall, 
          nParams, nConsts);
      }
      else
      {
        CryLog("%d: Compile %s %s (%d out of %d) ... ", nThread,
          mfProfileString(pInst->m_eClass), pSH->GetName(), nCombination, gRenDev->m_cEF.m_nCombinationsProcessOverall);
      }
    }
    else
    {
      //if (!bShaderThread)
        pSH->mfPrintCompileInfo(pInst);
#if !defined(XENON) && !defined(PS3)
      //else
      //if (pInst->m_pAsync)
      //  pInst->m_pAsync->m_bPendedPrint = true;
#endif
    }
  }
#if defined(DIRECT3D9) || defined(OPENGL)
  assert(pInst->m_pBindVars.size()<=30);
#endif

  mfGatherFXParameters(pInst, &pInst->m_pBindVars, &InstBindVars, pSH, bShaderThread ? 1 : 0, pFXShader); 

  if (pShader)
    mfCreateCacheItem(pInst, InstBindVars, (byte *)pShader->GetBufferPointer(), pShader->GetBufferSize(), pSH, bShaderThread);
#if !defined (XENON) && !defined(PS3)
  else
  if (!CParserBin::m_bXenon && !CParserBin::m_bPS3)
    mfCreateCacheItem(pInst, InstBindVars, NULL, 0, pSH, bShaderThread);
#endif





#if defined(DIRECT3D9) || defined(OPENGL)
  SAFE_RELEASE(pConstantTable);
  SAFE_RELEASE(pErrorMsgs);
#elif defined (DIRECT3D10)
  ID3D11ShaderReflection *pRFL = (ID3D11ShaderReflection *)pConstantTable;
  ID3D10Blob *pER = (ID3D10Blob *)pErrorMsgs;
  SAFE_RELEASE(pRFL);
  SAFE_RELEASE(pER);
#endif

  return true;
}

// Compile pixel/vertex shader for the current instance properties
bool CHWShader_D3D::mfActivate(uint32 nFlags, FXShaderToken* Table, DynArray<uint32>* pSHData)
{
  PROFILE_FRAME(Shader_HWShaderActivate);
//	LOADING_TIME_PROFILE_SECTION(iSystem);

  /*if (!stricmp(m_EntryFunc.c_str(), "ParticleVS"))
  {
    int nnn = 0;
  }*/

  bool bResult = true;
  SHWSInstance *pInst = m_pCurInst;
  if (mfIsValid(pInst, true) == ED3DShError_NotCompiled)
  {
    //if (!(m_Flags & HWSG_PRECACHEPHASE) && !(nFlags & HWSF_NEXT))
    //  mfSetHWStartProfile(nFlags);

    bool bCreate = false;
    char nameCache[256];
    float t0 = gEnv->pTimer->GetAsyncCurTime();

    /*if (CRenderer::CV_r_shaderspreactivate == 2 || (nFlags & HWSF_STORECOMBINATION))
    {
      strcpy(nameCache, GetName());
      char *s = strchr(nameCache, '(');
      if (s)
        s[0] = 0;
      gRenDev->m_cEF.mfInsertNewCombination(m_nMaskGenFX, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eSHClass, nameCache, 1);
      if (nFlags & HWSF_STORECOMBINATION)
        return false;
    }*/

    if (!m_pCache || m_pCache->m_bPlatformD3D11 != CParserBin::m_bD3D11 || m_pCache->m_bPlatformXenon != CParserBin::m_bXenon || m_pCache->m_bPlatformPS3 != CParserBin::m_bPS3)
    {
      mfGetDstFileName(pInst, this, nameCache, 256, 0);
      fpStripExtension(nameCache, nameCache);
      fpAddExtension(nameCache, ".fxcb");
      SAFE_RELEASE(m_pCache);
      m_pCache = mfInitCache(nameCache, this, true, m_CRC32, !CRenderer::CV_r_shadersuserfolder, true);
    }
    if (gRenDev->m_cEF.m_nCombinationsProcess >= 0 && !gRenDev->m_cEF.m_bActivatePhase)
    {
      mfGetDstFileName(pInst, this, nameCache, 256, 0);
      fpStripExtension(nameCache, nameCache);
      fpAddExtension(nameCache, ".fxcb");
      FXShaderCacheNamesItor it = m_ShaderCacheList.find(nameCache);
      if (it == m_ShaderCacheList.end())
        m_ShaderCacheList.insert(FXShaderCacheNamesItor::value_type(nameCache, m_CRC32));
    }
    uint32 nSize;
    SShaderCacheHeaderItem *pCacheItem = mfGetCacheItem(nFlags, nSize);
    if (pCacheItem && pCacheItem->m_Class != 255)
    {
      if (Table)
        mfGetCacheTokenMap(Table, pSHData, m_nMaskGenShader);
      if (((m_Flags & HWSG_PRECACHEPHASE) || gRenDev->m_cEF.m_nCombinationsProcess>=0) && !gRenDev->m_cEF.m_bActivatePhase)
      {
        byte *pData = (byte *)pCacheItem;
        SAFE_DELETE_ARRAY(pData);
        return true;
      }
      bool bRes = false;
      bRes = mfActivateCacheItem(pCacheItem, nSize, nFlags);
      byte *pData = (byte *)pCacheItem;
      SAFE_DELETE_ARRAY(pData);
      if (CRenderer::CV_r_shaderspreactivate == 2)
      {
        t0 = gEnv->pTimer->GetAsyncCurTime() - t0;
        iLog->Log(
          "Warning: Shader activation (%.3f ms): %s"
  #if defined(__GNUC__)
          "(%llx)"
  #else
          "(%I64x)"
  #endif
          "(%x)(%x)(%x)(%s)...", t0*1000.0f,
          GetName(), pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask,  mfProfileString(pInst->m_eClass));
      }
      if (bRes)
        return (pInst->m_Handle.m_pShader != NULL);
      pCacheItem = NULL;
    }
    else
    if (pCacheItem && pCacheItem->m_Class == 255)
    {
      return false;
    }
    else
    if (gRenDev->m_cEF.m_bActivatePhase)
    {
      if (CRenderer::CV_r_shadersdebug > 0)
      {
        iLog->Log(
          "Warning: Shader %s"
  #if defined(__GNUC__)
          "(%llx)"
  #else
          "(%I64x)"
  #endif
          "(%x)(%x)(%x)(%s) wasn't compiled before preactivating phase",
          GetName(), pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask,  mfProfileString(pInst->m_eClass));
      }
      return false;
    }









    //assert(!m_TokenData.empty());
    char *scr = NULL;
    if (nFlags & HWSF_PRECACHE)
      gRenDev->m_cEF.m_nCombinationsCompiled++;
    /*if (strstr(m_NameSourceFX.c_str(), "Cloak") && !strcmp(m_EntryFunc.c_str(), "Common_ZPassVS"))
    {
      int nnn = 0;
    }*/
    float fTime0 = iTimer->GetAsyncCurTime();
    LPD3DXBUFFER pShader = NULL;
    LPD3DXCONSTANTTABLE pConstantTable = NULL;
    LPD3DXBUFFER pErrorMsgs = NULL;
    DynArray<SCGBind> InstBindVars;
    m_Flags |= HWSG_WASGENERATED;
    if (!CRenderer::CV_r_shadersnocompile)
    {
      scr = mfGenerateScript(pInst, InstBindVars, nFlags, Table, pSHData);
		  assert (scr != NULL);
    }
		if (!scr)
		{
			Warning("Failed to activate shader %s", GetName());
			return false;
		}

    {
      PROFILE_FRAME(Shader_CompileHLSL);












      pShader = mfCompileHLSL(scr, &pConstantTable, &pErrorMsgs, nFlags, InstBindVars);




    }
    if (!pShader)
    {
      if (CRenderer::CV_r_shadersnocompile || pInst->IsAsyncCompiling())
        return false;
    }
    bResult = mfCreateShaderEnv(0, pInst, pShader, pConstantTable, pErrorMsgs, InstBindVars, this, false, gRenDev->m_RP.m_pShader, gRenDev->m_cEF.m_nCombinationsProcess, scr);
    bResult &= mfUploadHW(pShader, pInst, gRenDev->m_RP.m_pShader, nFlags);
    SAFE_RELEASE(pShader);

    fTime0 = iTimer->GetAsyncCurTime() - fTime0;
    //iLog->LogToConsole(" Time activate: %.3f", fTime0);
  }
  else
  if (pSHData)
    mfGetCacheTokenMap(Table, pSHData, m_nMaskGenShader);

  bool bSuccess = (mfIsValid(pInst, true) == ED3DShError_Ok);

  return bSuccess;
}

//////////////////////////////////////////////////////////////////////////

#ifdef SHADER_ASYNC_COMPILATION

#pragma warning(disable: 4355) // warning C4355: 'this' : used in base member initializer list

SShaderAsyncInfo CAsyncShaderTask::m_build_list;

CAsyncShaderTask::CAsyncShaderTask() : m_thread(this)
{




}

void CAsyncShaderTask::InsertPendingShader(SShaderAsyncInfo* pAsync)
{
  AUTO_LOCK(g_cAILock);
	pAsync->Link(&m_build_list);
	CryInterlockedIncrement(&SShaderAsyncInfo::s_nPendingAsyncShaders);



}

void CAsyncShaderTask::FlushPendingShaders()
{
  SShaderAsyncInfo *pAI, *pAI2, *pAINext;
	assert(m_flush_list.m_Prev == &m_flush_list && m_flush_list.m_Next == &m_flush_list); // the flush list must be empty - cleared by the shader compile thread
	if (m_build_list.m_Prev == &m_build_list && m_build_list.m_Next == &m_build_list)
		return; // the build list is empty, might need to do some assert here
  {
    AUTO_LOCK(g_cAILock);
    int n = 0;
    for (pAI=m_build_list.m_Prev; pAI!=&m_build_list; pAI=pAINext)
    {
      pAINext = pAI->m_Prev;
      pAI->Unlink();
      pAI->Link(&m_flush_list);
      n++;
      if (n == 32)
        break;
    }
	}

	// Sorting by distance
  if (gRenDev->m_cEF.m_nCombinationsProcess < 0)
	for (pAI=m_flush_list.m_Next; pAI!=&m_flush_list; pAI=pAI->m_Next)
	{
		pAINext = NULL;
		int nFrame = pAI->m_nFrame;
		float fDist = pAI->m_fMinDistance;
		for (pAI2=pAI->m_Next; pAI2!=&m_flush_list; pAI2=pAI2->m_Next)
		{
			if (pAI2->m_nFrame < nFrame)
				continue;
			if (pAI2->m_nFrame > nFrame || pAI2->m_fMinDistance < fDist)
			{
				pAINext = pAI2;
				nFrame = pAI2->m_nFrame;
				fDist = pAI2->m_fMinDistance;
			}
		}
		if (pAINext)
		{
			assert(pAI != pAINext);
			SShaderAsyncInfo *pAIP0 = pAI->m_Prev;
			SShaderAsyncInfo *pAIP1 = pAINext->m_Prev == pAI ? pAINext : pAINext->m_Prev;

			pAI->m_Next->m_Prev = pAI->m_Prev;
			pAI->m_Prev->m_Next = pAI->m_Next;
			pAI->m_Next = pAIP1->m_Next;
			pAIP1->m_Next->m_Prev = pAI;
			pAIP1->m_Next = pAI;
			pAI->m_Prev = pAIP1;

			pAI = pAINext;

			pAI->m_Next->m_Prev = pAI->m_Prev;
			pAI->m_Prev->m_Next = pAI->m_Next;
			pAI->m_Next = pAIP0->m_Next;
			pAIP0->m_Next->m_Prev = pAI;
			pAIP0->m_Next = pAI;
			pAI->m_Prev = pAIP0;
		}
	}

	for (pAI=m_flush_list.m_Next; pAI!=&m_flush_list; pAI=pAINext)
	{
		pAINext = pAI->m_Next;
		assert(pAI->m_bPending);
		bool bRes = CompileAsyncShader(pAI);
    {
      AUTO_LOCK(g_cAILock);

		  pAI->Unlink();
      pAI->m_bPending = 0;
      pAI->Link(&SShaderAsyncInfo::m_PendingListT);
    }
		CryInterlockedDecrement(&SShaderAsyncInfo::s_nPendingAsyncShaders);
	}
}

bool CAsyncShaderTask::PostCompile(SShaderAsyncInfo* pAsync)
{
  bool bResult = true;
  /*if (pAsync->m_nCombination < 0 && false)
  {
    CHWShader_D3D *pSH = pAsync->m_pShader;
    CHWShader_D3D::SHWSInstance *pInst = pSH->mfGetInstance(pAsync->m_nOwner, pSH->m_nMaskGenShader);
    bResult = CHWShader_D3D::mfCreateShaderEnv(m_nThread, pInst, pAsync->m_pDevShader, pAsync->m_pConstants, pAsync->m_pErrors, pAsync->m_InstBindVars, pSH, true, pAsync->m_pFXShader, pAsync->m_nCombination);
    assert(bResult == true);
  }
  else*/
  {
    pAsync->m_bPendedEnv = true;
    pAsync->m_nThread = m_nThread;
  }
  return bResult;
}

bool CAsyncShaderTask::CompileAsyncShader(SShaderAsyncInfo* pAsync)
{
	bool bResult = true;
	if(CRenderer::CV_r_shadersremotecompiler)
	{
		const char* pCompiler = gRenDev->m_cEF.mfGetShaderCompileFlags(pAsync->m_eClass);

		//////////////////////////////////////////////////////////////////////////
		// Generate request line text.
		CHWShader_D3D *pSH = pAsync->m_pShader;
		char szShaderGenName[512];
		strcpy_s(szShaderGenName, pSH->GetName());
		char *s = strchr(szShaderGenName, '(');
		if (s)
      s[0] = 0;
    string RequestLine;
		gRenDev->m_cEF.mfInsertNewCombination(pSH->m_nMaskGenFX, pAsync->m_RTMask, pAsync->m_LightMask, pAsync->m_MDMask, pAsync->m_MDVMask, pAsync->m_eClass, szShaderGenName, 0, &RequestLine, false);
		//////////////////////////////////////////////////////////////////////////

		std::vector<uint8> Data;
		if(!NRemoteCompiler::CShaderSrv::Instance().Compile(Data,pAsync->m_Profile,pAsync->m_Text.c_str(),pAsync->m_Name.c_str(),pCompiler,RequestLine.c_str()))
		{
#if defined (DIRECT3D9) || defined(OPENGL)
			D3DXCreateBuffer(sizeof("D3DXCompileShader failed"), &pAsync->m_pErrors);
			DWORD *pBuf = (DWORD *)pAsync->m_pErrors->GetBufferPointer();
#elif defined (DIRECT3D10)
			D3D10CreateBlob(sizeof("D3DXCompileShader failed"), (LPD3D10BLOB *)&pAsync->m_pErrors);
			DWORD *pBuf = (DWORD *)pAsync->m_pErrors->GetBufferPointer();
			memcpy(pBuf,"D3DXCompileShader failed",sizeof("D3DXCompileShader failed"));
#endif
			string sErrorText;
			sErrorText.reserve(Data.size());
			for (uint32 i = 0; i < Data.size(); i++)
				sErrorText += Data[i];
			pAsync->m_Errors += "Remote Shader Compilation failed: " + sErrorText;
			return false;
		}

		HRESULT hr;
#if defined (DIRECT3D9) || defined(OPENGL)
		hr = D3DXCreateBuffer(Data.size(), &pAsync->m_pDevShader);
		LPD3DXBUFFER pShader = pAsync->m_pDevShader;
		DWORD *pBuf = (DWORD *)pShader->GetBufferPointer();
		memcpy(pBuf,&Data[0],Data.size());

		if (!CParserBin::m_bPS3 && !CParserBin::m_bD3D11)
			hr = D3DXGetShaderConstantTable(pBuf, &pAsync->m_pConstants);

		if (hr == S_OK)
		{
			SAFE_RELEASE(pAsync->m_pErrors);
      bResult = PostCompile(pAsync);
		}
		else
		{
			pAsync->m_pDevShader	=	0;
			pAsync->m_Errors += "D3DXCompileShader failed";
		}

		bResult	=	hr==S_OK;

#elif defined (DIRECT3D10)
		D3D10CreateBlob(Data.size(), (LPD3D10BLOB *)&pAsync->m_pDevShader);
		LPD3D10BLOB pShader = (LPD3D10BLOB)*&pAsync->m_pDevShader;
		DWORD *pBuf = (DWORD *)pShader->GetBufferPointer();
		memcpy(pBuf,&Data[0],Data.size());

		pAsync->m_pDevShader = (LPD3DXBUFFER)pShader;
		pBuf = (DWORD *)pShader->GetBufferPointer();
		UINT nSize = pShader->GetBufferSize();

		ID3D11ShaderReflection *pShaderReflection;
		hr = D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
		if (SUCCEEDED(hr))
		{
			pAsync->m_pConstants = (LPD3DXCONSTANTTABLE)pShaderReflection;
      bResult = PostCompile(pAsync);
		}
		else
		{
			pAsync->m_pDevShader	=	0;
			assert(0);
		}
#endif

	}
	else
	{
#if defined (DIRECT3D9) || defined(OPENGL)
		int nFlags = D3DXSHADER_PACKMATRIX_ROWMAJOR;





	nFlags |= D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY;


		HRESULT hr = D3DXCompileShader(pAsync->m_Text.c_str(), pAsync->m_Text.size(), NULL, NULL, pAsync->m_Name.c_str(), pAsync->m_Profile.c_str(), nFlags, &pAsync->m_pDevShader, &pAsync->m_pErrors, &pAsync->m_pConstants); 
		if (FAILED(hr))
		{
			if (pAsync->m_pErrors)
			{
				const char *err = (const char *)pAsync->m_pErrors->GetBufferPointer();
				pAsync->m_Errors += err;
			}
			else
			{ 
				pAsync->m_Errors += "D3DXCompileShader failed";
			}
			bResult = false;
		}
		else
		{
			SAFE_RELEASE(pAsync->m_pErrors);
      bResult = PostCompile(pAsync);
		}
#elif defined (DIRECT3D10)
		const char *Name = pAsync->m_pShader ? pAsync->m_pShader->GetName() : "Unknown";
		HRESULT hr = S_OK;
		hr = D3DX11CompileFromMemory(pAsync->m_Text.c_str(),
			pAsync->m_Text.size(),
			Name,
			NULL,
			NULL,
			pAsync->m_Name.c_str(),
			pAsync->m_Profile.c_str(),
			D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY,
			0,
			NULL,
			(ID3D10Blob **)&pAsync->m_pDevShader,
			(ID3D10Blob **)&pAsync->m_pErrors, &hr);
		if (FAILED(hr) || !pAsync->m_pDevShader)
		{
			if (pAsync->m_pErrors)
			{
				const char *err = (const char *)pAsync->m_pErrors->GetBufferPointer();
				pAsync->m_Errors += err;
			}
			else
			{
				pAsync->m_Errors += "D3DXCompileShader failed";
			}
			bResult = false;
		}
		else
		{
			ID3D11ShaderReflection *pShaderReflection;
			UINT *pData = (UINT *)pAsync->m_pDevShader->GetBufferPointer();
			UINT nSize = pAsync->m_pDevShader->GetBufferSize();
			hr = D3DReflect(pData, nSize, IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
			if (SUCCEEDED(hr))
			{
				pAsync->m_pConstants = (LPD3DXCONSTANTTABLE)pShaderReflection;
        bResult = PostCompile(pAsync);
			}
			else
			{
				assert(0);
			}
		}
#endif
	}
  return bResult;
}

void CAsyncShaderTask::CShaderThread::Run()
{
	CryThreadSetName( -1, SHADER_THREAD_NAME );		





	while (!m_quit)
	{
		m_task->FlushPendingShaders();




		if (!CRenderer::CV_r_shadersasynccompiling)
			Sleep(250);
		else
			Sleep(25);

	}
}

#endif

//===============================================================================================
// Export/Import

#ifdef SHADERS_SERIALIZING

bool STexSampler::Export(SShaderSerializeContext& SC)
{
  bool bRes = true;

  SSTexSampler TS;
  TS.m_bHasRT = 0;
  TS.m_nsName = SC.Strings.Num();
  SC.Strings.AddString(m_Name.c_str());

  TS.m_nsNameTexture = SC.Strings.Num();
  SC.Strings.AddString(m_Texture.c_str());

  TS.m_eTexType = m_eTexType;
  TS.m_nFlags = m_nFlags;
  TS.m_nSamplerSlot = m_nSamplerSlot;
  TS.m_nTexFlags = m_nTexFlags;
  if (m_nTexState > 0)
  {
    TS.m_bTexState = 1;
    STexState *pTS = &CTexture::s_TexStates[m_nTexState];
    memcpy(&TS.ST, &CTexture::s_TexStates[m_nTexState], sizeof(TS.ST));
    TS.ST.m_pDeviceState = NULL;
  }

  if (m_pTarget)
    TS.m_bHasRT = 1;
  sAddData(SC.Data, TS);

  if (m_pTarget)
  {
    SHRenderTarget *pRT = m_pTarget;
    SSHRenderTarget RT;
    RT.m_eOrder = pRT->m_eOrder;
    RT.m_nProcessFlags = pRT->m_nProcessFlags;
    RT.m_nsTargetName = SC.Strings.Num();
    SC.Strings.AddString(pRT->m_TargetName.c_str());
    RT.m_nWidth = pRT->m_nWidth;
    RT.m_nHeight = pRT->m_nHeight;
    RT.m_eTF = pRT->m_eTF;
    RT.m_nIDInPool = pRT->m_nIDInPool;
    RT.m_eUpdateType = pRT->m_eUpdateType;
    RT.m_bTempDepth = pRT->m_bTempDepth;
    RT.m_ClearColor = pRT->m_ClearColor;
    RT.m_fClearDepth = pRT->m_fClearDepth;
    RT.m_nFlags = pRT->m_nFlags;
    RT.m_nFilterFlags = pRT->m_nFilterFlags;
    sAddData(SC.Data, RT);
  }

  return bRes;
}
bool STexSampler::Import(SShaderSerializeContext& SC, byte *& pData)
{
  bool bRes = true;

  SSTexSampler *pTS = (SSTexSampler *)pData;
  pData += sizeof(SSTexSampler);

  m_Name = sString(pTS->m_nsName, SC.Strings);
  m_Texture = sString(pTS->m_nsNameTexture, SC.Strings);

  m_eTexType = pTS->m_eTexType;
  m_nFlags = pTS->m_nFlags;
  m_nSamplerSlot = pTS->m_nSamplerSlot;
  m_nTexFlags = pTS->m_nTexFlags;
  if (pTS->m_bTexState)
    m_nTexState = CTexture::GetTexState(pTS->ST);
  if (pTS->m_bHasRT)
  {
    SSHRenderTarget *pRT = (SSHRenderTarget *)pData;
    pData += sizeof(SSHRenderTarget);
    SHRenderTarget *pDst = new SHRenderTarget;

    pDst->m_eOrder = pRT->m_eOrder;
    pDst->m_nProcessFlags = pRT->m_nProcessFlags;
    pDst->m_TargetName = sString(pRT->m_nsTargetName, SC.Strings);
    pDst->m_nWidth = pRT->m_nWidth;
    pDst->m_nHeight = pRT->m_nHeight;
    pDst->m_eTF = pRT->m_eTF;
    pDst->m_nIDInPool = pRT->m_nIDInPool;
    pDst->m_eUpdateType = pRT->m_eUpdateType;
    pDst->m_bTempDepth = pRT->m_bTempDepth != 0;
    pDst->m_ClearColor = pRT->m_ClearColor;
    pDst->m_fClearDepth = pRT->m_fClearDepth;
    pDst->m_nFlags = pRT->m_nFlags;
    pDst->m_nFilterFlags = pRT->m_nFilterFlags;
    m_pTarget = pDst;
  }

  PostLoad();

  return bRes;
}

bool SFXParam::Export(SShaderSerializeContext& SC)
{
  bool bRes = true;

  SSFXParam PR;
  PR.m_nsName = SC.Strings.Num();
  SC.Strings.AddString(m_Name.c_str());

  PR.m_nsAnnotations = SC.Strings.Num();
  SC.Strings.AddString(m_Annotations.c_str());

  PR.m_nsAssign = SC.Strings.Num();
  SC.Strings.AddString(m_Assign.c_str());

  PR.m_nsValues = SC.Strings.Num();
  SC.Strings.AddString(m_Values.c_str());

  PR.m_bAffected = m_bAffected;
  PR.m_bWasMerged = m_bWasMerged;
  PR.m_eType = m_eType;
  PR.m_nCB = m_nCB;
  PR.m_nComps = m_nComps;
  PR.m_nFlags = m_nFlags;
  PR.m_nParameters = m_nParameters;
  PR.m_nRegister[0] = m_nRegister[0];
  PR.m_nRegister[1] = m_nRegister[1];
  PR.m_nRegister[2] = m_nRegister[2];

  sAddData(SC.Data, PR);

  return bRes;
}
bool SFXParam::Import(SShaderSerializeContext& SC, byte *& pData)
{
  bool bRes = true;

  SSFXParam *pPR = (SSFXParam *)pData;
  pData += sizeof(SSFXParam);

  m_Name = sString(pPR->m_nsName, SC.Strings);
  m_Annotations = sString(pPR->m_nsAnnotations, SC.Strings);
  m_Assign = sString(pPR->m_nsAssign, SC.Strings);
  m_Values = sString(pPR->m_nsValues, SC.Strings);

  m_bAffected = pPR->m_bAffected;
  m_bWasMerged = pPR->m_bWasMerged;
  m_eType = pPR->m_eType;
  m_nCB = pPR->m_nCB;
  m_nComps = pPR->m_nComps;
  m_nFlags = pPR->m_nFlags;
  m_nParameters = pPR->m_nParameters;
  m_nRegister[0] = pPR->m_nRegister[0];
  m_nRegister[1] = pPR->m_nRegister[1];
  m_nRegister[2] = pPR->m_nRegister[2];

  return bRes;
}


bool CHWShader_D3D::ExportSamplers(SCHWShader& SHW, SShaderSerializeContext& SC)
{
  bool bRes = true;

  int i;

  SHW.m_nSamplers = m_Samplers.size();
  for (i=0; i<m_Samplers.size(); i++)
  {
    STexSampler& TS = m_Samplers[i];
    bRes &= TS.Export(SC);
  }

  return bRes;
}
bool CHWShader::ImportSamplers(SShaderSerializeContext& SC, SCHWShader *pSHW, byte *& pData, DynArray<STexSampler>& Samplers)
{
  bool bRes = true;
  int i;

  for (i=0; i<pSHW->m_nSamplers; i++)
  {
    STexSampler TS;
    bRes &= TS.Import(SC, pData);
    if (bRes)
      Samplers.push_back(TS);
  }
  return bRes;
}

bool CHWShader_D3D::ExportParams(SCHWShader& SHW, SShaderSerializeContext& SC)
{
  bool bRes = true;

  int i;

  SHW.m_nParams = m_Params.size();
  for (i=0; i<m_Params.size(); i++)
  {
    SFXParam& PR = m_Params[i];
    bRes &= PR.Export(SC);
  }

  return bRes;
}
bool CHWShader::ImportParams(SShaderSerializeContext& SC, SCHWShader *pSHW, byte *& pData, DynArray<SFXParam>& Params)
{
  bool bRes = true;
  int i;

  for (i=0; i<pSHW->m_nParams; i++)
  {
    SFXParam PR;
    bRes &= PR.Import(SC, pData);
    if (bRes)
      Params.push_back(PR);
  }
  return bRes;
}

bool CHWShader_D3D::Export(SShaderSerializeContext& SC)
{
  bool bRes = true;

  SCHWShader SHW;

  char str[256];
  strcpy(str, GetName());
  char *c = strchr(str, '(');
  if (c)
    c[0] = 0;
  SHW.m_nsName = SC.Strings.Num();
  SC.Strings.AddString(str);

  SHW.m_nsNameSourceFX = SC.Strings.Num();
  SC.Strings.AddString(m_NameSourceFX.c_str());

  SHW.m_nsEntryFunc = SC.Strings.Num();
  SC.Strings.AddString(m_EntryFunc.c_str());

  SHW.m_eSHClass = m_eSHClass;
  SHW.m_eHWProfile = m_eHWProfile;
  SHW.m_dwShaderType = m_dwShaderType;
  SHW.m_nMaskGenFX = m_nMaskGenFX;
  SHW.m_nMaskGenShader = m_nMaskGenShader;

  FXShaderToken *pMap = NULL;
  DynArray<uint32> *pData = &m_TokenData;
  if (!pData->size())
  {
    if (m_pCache)
      mfGetCacheTokenMap(pMap, pData, m_nMaskGenShader);
    else
      assert(0);
  }
  if (!pMap)
    return false;
  SHW.m_nTokens = pData->size();
  SHW.m_nTableEntries = pMap->size();

  uint32 nOffsHWS = SC.Data.Num();
  sAddData(SC.Data, SHW);
  uint32 nOffs = 0;
  sAddData(SC.Data, *pData, nOffs);

  FXShaderTokenItor itor;
  for (itor=pMap->begin(); itor!=pMap->end(); itor++)
  {
    uint32 nTok = itor->first;
    if (CParserBin::m_bEndians)
      SwapEndian(nTok, eBigEndian);
    SC.Data.Copy((byte *)&nTok, sizeof(DWORD));
    uint32 nSize = itor->second.SToken.size()+1;
    SC.Data.Align4Copy((byte *)itor->second.SToken.c_str(), nSize);
  }

  bRes &= ExportSamplers(SHW, SC);
  bRes &= ExportParams(SHW, SC);

  if (bRes)
  {
    SCHWShader *pSH = (SCHWShader *)&SC.Data[nOffsHWS];
    memcpy(pSH, &SHW, sizeof(SHW));
  }

  return bRes;
}

static CHWShader *mfForName(const char *name, const char *nameSource, uint32 CRC32, DynArray<STexSampler>& Samplers, DynArray<SFXParam>& Params, uint32 dwEntryFunc, EHWShaderClass eClass, EHWSProfile eSHV, DynArray<uint32>& SHData, FXShaderToken& m_Table, uint32 dwType, uint64 nMaskGen=0, uint64 nMaskGenFX=0);

CHWShader *CHWShader::Import(SShaderSerializeContext& SC, int nOffs, uint32 CRC32)
{
  if (nOffs < 0)
    return NULL;

  CHWShader *pSH = NULL;
  SCHWShader *pSHW = (SCHWShader *)&SC.Data[nOffs];
  byte *pData = &SC.Data[nOffs+sizeof(SCHWShader)];

  const char *szName = sString(pSHW->m_nsName, SC.Strings);
  const char *szNameSource = sString(pSHW->m_nsNameSourceFX, SC.Strings);
  const char *szNameEntry = sString(pSHW->m_nsEntryFunc, SC.Strings);

  DynArray<uint32> SHData;
  SHData.resize(pSHW->m_nTokens);
  memcpy(&SHData[0], pData, pSHW->m_nTokens*sizeof(uint32));
  pData += pSHW->m_nTokens*sizeof(uint32);

  FXShaderToken Table;
  nOffs = 0;
  for (int i=0; i<pSHW->m_nTableEntries; i++)
  {
    char *pStr = (char *)&pData[nOffs+sizeof(DWORD)];
    DWORD nToken = *(DWORD *)&pData[nOffs];
    if (CParserBin::m_bEndians)
      SwapEndian(nToken, eBigEndian);
    int nLen = strlen(pStr)+1;
    if (nLen & 3)
      nLen += 4 - (nLen & 3);
    STokenD TD;
    TD.SToken = pStr;
    Table.insert(FXShaderTokenItor::value_type(nToken, TD));
    nOffs += sizeof(DWORD) + nLen;
  }
  pData += nOffs;

  DynArray<STexSampler> Samplers;
  ImportSamplers(SC, pSHW, pData, Samplers);

  DynArray<SFXParam> Params;
  ImportParams(SC, pSHW, pData, Params);

  bool bPrecache = (SC.SSR.m_Flags & EF_PRECACHESHADER) != 0;

  pSH = CHWShader::mfForName(szName, szNameSource, CRC32, Samplers, Params, szNameEntry, pSHW->m_eSHClass, pSHW->m_eHWProfile, SHData, &Table, pSHW->m_dwShaderType, pSHW->m_nMaskGenShader, pSHW->m_nMaskGenFX, bPrecache);

  return pSH;
}
#else
bool CHWShader_D3D::Export(SShaderSerializeContext& SC)
{
  return false;
}
#endif

const char * CHWShader_D3D::mfGetSharedActivatedCombinations(bool bForLevel)
{
  TArray <char> Combinations;

  int i, j, n;
  InstanceMapItor it;
  for (it=m_SharedInsts.begin(); it!=m_SharedInsts.end(); it++)
  {
    SHWSSharedList *pList = it->second;
    CCryName Name = it->first;
    if (!pList)
      continue;
    if (pList->m_SharedNames.size())
    {
      for (i=0; i<pList->m_SharedInsts.size(); i++)
      {
        SHWSSharedInstance *pSInst = &pList->m_SharedInsts[i];
        for (j=0; j<pSInst->m_Insts.size(); j++)
        {
          SHWSInstance *pInst = &pSInst->m_Insts[j];
          char name[256];
          for (n=0; n<pList->m_SharedNames.size(); n++)
          {
            sprintf(name, "%s@%s", pList->m_SharedNames[n].m_Name.c_str(), Name.c_str());
            string str;
            gRenDev->m_cEF.mfInsertNewCombination(pSInst->m_GLMask, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eClass, name, 0, &str, false);
            assert (str.size());
            if (str.size())
            {
              assert(str[0] == '<' && str[2] == '>');
              string s;
              if (str[0] == '<' && str[2] == '>')
                s.Format("<%d>%s", pInst->m_nUsed, &str[3]);
              else
                s = str;
              Combinations.Copy(s.c_str(), s.size());
              Combinations.Copy("\n", 1);
            }
          }
        }
      }
    }
  }
  if (!Combinations.Num())
    return NULL;
  char *pPtr = new char [Combinations.Num()+1];
  memcpy(pPtr, &Combinations[0], Combinations.Num());
  pPtr[Combinations.Num()] = 0;
  return pPtr;
}

const char * CHWShader_D3D::mfGetActivatedCombinations(bool bForLevel)
{
  TArray <char> Combinations;
  char *pPtr = NULL;
  int i;

  for (i=0; i<m_Insts.size(); i++)
  {
    SHWSInstance *pInst = &m_Insts[i];
    char name[256];
    strcpy(name, GetName());
    char *s = strchr(name, '(');
    if (s)
      s[0] = 0;
    string str;
    gRenDev->m_cEF.mfInsertNewCombination(m_nMaskGenFX, pInst->m_RTMask, pInst->m_LightMask, pInst->m_MDMask, pInst->m_MDVMask, pInst->m_eClass, name, 0, &str, false);
    assert (str.size());
    if (str.size())
    {
      assert(str[0] == '<' && str[2] == '>');
      string s1;
      if (str[0] == '<' && str[2] == '>')
        s1.Format("<%d>%s", pInst->m_nUsed, &str[3]);
      else
        s1 = str;
      Combinations.Copy(s1.c_str(), s1.size());
      Combinations.Copy("\n", 1);
    }
  }

  if (!Combinations.Num())
    return NULL;
  pPtr = new char [Combinations.Num()+1];
  memcpy(pPtr, &Combinations[0], Combinations.Num());
  pPtr[Combinations.Num()] = 0;
  return pPtr;
}

const char *CHWShader::GetCurrentShaderCombinations(bool bForLevel)
{
  TArray <char> Combinations;
  char *pPtr = NULL;
  CCryName Name;
  SResourceContainer *pRL;

  Name = CHWShader::mfGetClassName(eHWSC_Vertex);
  pRL = CBaseResource::GetResourcesForClass(Name);
  int nVS = 0;
  int nPS = 0;
  if (pRL)
  {
    ResourcesMapItor itor;
    for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
    {
      CHWShader *vs = (CHWShader *)itor->second;
      if (!vs)
        continue;
      const char *szCombs = vs->mfGetActivatedCombinations(bForLevel);
      if (!szCombs)
        continue;
      Combinations.Copy(szCombs, strlen(szCombs));
      delete [] szCombs;
      nVS++;
    }
  }

  Name = CHWShader::mfGetClassName(eHWSC_Pixel);
  pRL = CBaseResource::GetResourcesForClass(Name);
  int n = 0;
  if (pRL)
  {
    ResourcesMapItor itor;
    for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
    {
      CHWShader *ps = (CHWShader *)itor->second;
      if (!ps)
        continue;
      const char *szCombs = ps->mfGetActivatedCombinations(bForLevel);
      if (!szCombs)
        continue;
      Combinations.Copy(szCombs, strlen(szCombs));
      delete [] szCombs;
      nPS++;
    }
  }

  const char *szCombs = CHWShader_D3D::mfGetSharedActivatedCombinations(bForLevel);
  if (szCombs)
  {
    Combinations.Copy(szCombs, strlen(szCombs));
    delete [] szCombs;
  }

  if (!Combinations.Num())
    return NULL;
  pPtr = new char [Combinations.Num()+1];
  memcpy(pPtr, &Combinations[0], Combinations.Num());
  pPtr[Combinations.Num()] = 0;
  return pPtr;
}

bool CHWShader::SetCurrentShaderCombinations(const char *szCombinations, bool bForLevel)
{
  bool bRes = true;

  if (CRenderer::CV_r_shaderspreactivate == 3)
  {
    return gRenDev->m_cEF.mfPreactivateShaders2();
  }
  FXShaderCacheCombinations Combinations;
  gRenDev->m_cEF.mfInitShadersCache(bForLevel, &Combinations, szCombinations);
  if (CRenderer::CV_r_shaderspreactivate == 2)
    gRenDev->m_cEF.mfMergeShadersCombinations(&Combinations, 1);
  if (!Combinations.size())
    return false;
  gRenDev->m_cEF.mfPreactivateShaders(&Combinations);

  return bRes;
}

