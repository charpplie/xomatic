/*=============================================================================
  ShaderSerialize.h : Shaders serialization declarations.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#ifndef __SHADERSERIALIZE_H__
#define __SHADERSERIALIZE_H__

template <typename T>  void sAddData(TArray<byte>& Dst, T& Src)
{
  int nSize = sizeof(T);
  byte *pDst = Dst.Grow(nSize);
  memcpy(pDst, &Src, nSize);
}
template <typename T> void sAddData(TArray<byte>& Dst, TArray<T>& Src, uint32& nOffs)
{
  nOffs = Dst.Num();
  int nSize = sizeof(T)*Src.Num();
  if (!nSize)
    return;
  byte *pDst = Dst.Grow(nSize);
  memcpy(pDst, &Src[0], nSize);
}
template <typename T> void sAddData(TArray<byte>& Dst, DynArray<T>& Src, uint32& nOffs)
{
  nOffs = Dst.Num();
  int nSize = sizeof(T)*Src.size();
  byte *pDst = Dst.Grow(nSize);
  if (CParserBin::m_bEndians)
  {
    for (int i=0; i<Src.size(); i++)
    {
      T d = Src[i];
      SwapEndian(d, eBigEndian);
      memcpy(pDst, &d, sizeof(T));
    }
  }
  else
    memcpy(pDst, &Src[0], nSize);
}


struct SSShaderCacheHeader
{
  int m_SizeOf;
  char m_szVer[16];
  int m_MajorVer;
  int m_MinorVer;
  uint32 m_CRC32;
  SSShaderCacheHeader()
  {
    memset(this, 0, sizeof(*this));
  }
  AUTO_STRUCT_INFO
};


struct SSShaderRes
{
  int m_nRefCount;
  CResFile *m_pRes[2];
  SSShaderCacheHeader m_Header[2];
  bool m_bReadOnly[2];
  SSShaderRes()
  {
    m_nRefCount = 1;
    m_pRes[0] = NULL;
    m_pRes[1] = NULL;
    m_bReadOnly[0] = true;
    m_bReadOnly[1] = true;
  }
};
typedef std::map<CCryName, SSShaderRes*> FXSShaderRes;
typedef FXSShaderRes::iterator FXSShaderResItor;

struct SSShader
{
  EShaderDrawType m_eSHDType; 

  uint32 m_Flags;             
  uint32 m_Flags2;      
  uint32 m_nMDV;        

  EVertexFormat m_eVertexFormat;     // Base vertex format for the shader (see VertexFormats.h)
  ECull m_eCull;                     // Global culling type

  EShaderType m_eShaderType;
  uint64 m_nMaskGenFX;

  uint32 m_nTechniques;
  uint32 m_nPasses;
  uint32 m_nPublicParams;
  uint32 m_nDataSize;
  uint32 m_nStringsSize;

  uint32 m_nParamsOffset;
  uint32 m_nTechOffset;
  uint32 m_nPassOffset;
  uint32 m_nStringsOffset;
  uint32 m_nDataOffset;

  SSShader()
  {
    memset(this, 0, sizeof(*this));
  }
};

struct SSShaderParam
{
  char m_Name[32];
  EParamType m_Type;
  UParamVal m_Value;
  int m_nScriptOffs;
};

struct SSShaderTechnique
{
  int m_nNameOffs;
  int m_nPassesOffs;
  int m_nPasses;
  int m_Flags;                 
  int8 m_nTechnique[TTYPE_MAX];
  int m_nREsOffs;
  int m_nREs;
  uint32 m_nPreprocessFlags;

  SSShaderTechnique()
  {
    memset(this, 0, sizeof(*this));
  }
};

struct SSShaderPass
{
  uint32 m_RenderState;
  int8 m_eCull;
  uint8 m_AlphaRef;
  uint16 m_PassFlags;

  uint32 m_nVShaderOffs;
  uint32 m_nPShaderOffs;
  uint32 m_nGShaderOffs;
};

struct SSLightEval
{

};

struct SCHWShader
{
  uint32 m_nsNameSourceFX;
  uint32 m_nsName;
  uint32 m_nsEntryFunc;
  EHWShaderClass m_eSHClass;
  //EHWSProfile m_eHWProfile;
  uint64 m_nMaskGenShader;
  uint64 m_nMaskGenFX;
  uint32 m_nTokens;
  uint32 m_nTableEntries;
  uint32 m_nSamplers;
  uint32 m_nParams;
  uint32 m_nTableSize;
  uint32 m_dwShaderType;
};

struct SSTexSampler
{
  int m_nsName;
  int m_nsNameTexture;

  int m_eTexType;
  int m_nSamplerSlot;
  uint32 m_nFlags;
  uint32 m_nTexFlags;
  uint32 m_bHasRT;
  uint32 m_bTexState;
  STexState ST;

  SSTexSampler()
  {
    memset(this, 0, sizeof(*this));
  }
};

struct SSHRenderTarget
{
  ERenderOrder m_eOrder;
  int m_nProcessFlags;     // FSPR_ flags
  uint32 m_nsTargetName;
  int m_nWidth;
  int m_nHeight;
  ETEX_Format m_eTF;
  int m_nIDInPool;
  ERTUpdate m_eUpdateType;
  uint32 m_bTempDepth;
  ColorF m_ClearColor;
  float m_fClearDepth;
  uint32 m_nFlags;
  uint32 m_nFilterFlags;
};

struct SSFXParam
{
  int m_nsName;        // Parameter name
  uint32 m_nFlags;
  short m_nParameters;  // Number of paramters
  short m_nComps;       // Number of components in single parameter
  uint32 m_nsAnnotations; // Additional parameters (between <>)
  uint32 m_nsAssign;      // Parameter app handling type (after ':')
  uint32 m_nsValues;      // Parameter values (after '=')
  byte   m_bWasMerged;
  byte   m_bAffected;
  byte   m_eType;       // EParamType
  int8   m_nCB;
  short  m_nRegister[3]; // VS, PS, GS
};

struct SShaderSerializeContext
{
  SSShader SSR;
  TArray<SSShaderParam> Params;
  TArray<SSShaderTechnique> Techniques;
  TArray<SSShaderPass> Passes;
  TArray<char> Strings;
  TArray<byte> Data;
};


class CShaderSerialize
{
friend class CShaderMan;
friend class CShaderManBin;

private:
  bool _OpenSResource(float fVersion, SSShaderRes *pSR, CShader *pSH, int nCache, CResFile *pRF, bool bReadOnly);
  bool OpenSResource(const char *szName,  SSShaderRes *pSR, CShader *pSH, bool bDontUseUserFolder, bool bReadOnly);
  bool CreateSResource(CShader *pSH, SSShaderRes *pSR, CCryName& SName, bool bDontUseUserFolder, bool bReadOnly);
  SSShaderRes *InitSResource(CShader *pSH, bool bDontUseUserFolder, bool bReadOnly);

  bool ExportHWShader(CHWShader *pShader, struct SShaderSerializeContext& SC);

  CHWShader *ImportHWShader(SShaderSerializeContext& SC, int nOffs, uint32 CRC32);

  bool ExportShader(CShader *pSH);
  bool ImportShader(CShader *pSH);

  FXSShaderRes m_SShaderResources;
};

_inline const char *sString(int nOffs, TArray<char>& Strings)
{
  return &Strings[nOffs];
}

#endif

