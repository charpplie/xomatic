/*=============================================================================
  ShaderCache.h : Shaders cache related declarations.
  Copyright (c) 2001-2009 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#ifndef __SHADERCACHE_H__
#define __SHADERCACHE_H__

struct SPreprocessMasks
{
  uint64 nRT, nRTSet;
  uint64 nGL, nGLSet;
  uint32 nLT, nLTSet;
  uint32 nMD, nMDSet;
  uint32 nMDV, nMDVSet;
};

typedef std::map<uint32, uint32> MapPreprocessFlags;
typedef MapPreprocessFlags::iterator MapPreprocessFlagsItor;

struct SPreprocessNode
{
  std::vector<uint32> m_Expression;
  int m_nNode;
  uint64 m_RTMask;
  uint64 m_GLMask;
  uint32 m_LTMask;
  uint32 m_MDMask;
  uint32 m_MDVMask;
  std::vector<SPreprocessNode *> m_Nodes[2];
  short m_nCode[2];   // 0 - No Code, 1 - Has Code, -1 - Depends
  SPreprocessNode() 
  {
    m_nNode = 0;
    m_RTMask = 0;
    m_GLMask = 0;
    m_LTMask = 0;
    m_MDMask = 0;
    m_MDVMask = 0;
    m_nCode[0] = m_nCode[1] = 0;
  }
  ~SPreprocessNode()
  {
    uint32 i;
    for (i=0; i<m_Nodes[0].size(); i++)
    {
      SAFE_DELETE(m_Nodes[0][i]);
    }
    for (i=0; i<m_Nodes[1].size(); i++)
    {
      SAFE_DELETE(m_Nodes[1][i]);
    }
  }
};

struct SPreprocessFlagDesc
{
  CCryName m_Name;
  uint64 m_nFlag;
  bool m_bWasReferenced;
  SPreprocessFlagDesc() { m_nFlag = 0; m_bWasReferenced = true; }
};

struct SPreprocessTree
{
  static std::vector<SPreprocessFlagDesc> m_GLDescs;
  static std::vector<SPreprocessFlagDesc> m_RTDescs;
  static std::vector<SPreprocessFlagDesc> m_LTDescs;
  std::vector<SPreprocessFlagDesc> m_GLDescsLocal;
  std::vector<SPreprocessFlagDesc> m_MDDescs;
  std::vector<SPreprocessFlagDesc> m_MDVDescs;

  static MapPreprocessFlags m_MapFlags;
  MapPreprocessFlags m_MapFlagsLocal;
 
  std::vector<SPreprocessNode *> m_Root;
  ~SPreprocessTree()
  {
    uint32 i;
    for (i=0; i<m_Root.size(); i++)
    {
      SAFE_DELETE(m_Root[i]);
    }
    m_Root.clear();
  }
  CShader *m_pSH;
};

//==========================================================================================================================

struct SCacheCombination
{
  CCryName Name;
  CCryName CacheName;
  uint64 nGL;
  //uint32 nGLMaskAuto;
  //uint32 nGLAuto;
  int64 nRT;
  int nLT;
  int nMD;
  int nMDV;
  EHWShaderClass eCL;
  SCacheCombination()
  {
    nGL = 0;
    //nGLMaskAuto = 0;
    //nGLAuto = 0;
    nRT = 0;
    nLT = 0;
    nMD = 0;
    nMDV = 0;
    eCL = eHWSC_Vertex;
  }

	void GetMemoryUsage( ICrySizer *pSizer ) const {}
};

struct SShaderGenComb
{
  CCryName Name;
  SShaderGen *pGen;
  SShaderGenComb()
  {
    pGen = NULL;
  }
  ~SShaderGenComb()
  {
    SAFE_RELEASE(pGen);
  }

  _inline SShaderGenComb (const SShaderGenComb& src)
  {
    Name = src.Name;
    pGen = src.pGen;
    if (pGen)
      pGen->m_nRefCount++;
  }
  _inline SShaderGenComb& operator = (const SShaderGenComb& src)
  {
    this->~SShaderGenComb();
    new(this) SShaderGenComb(src);
    return *this;
  }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(Name);
		pSizer->AddObject(pGen);
	}
};

typedef std::map<CCryName, SCacheCombination> FXShaderCacheCombinations;
typedef FXShaderCacheCombinations::iterator FXShaderCacheCombinationsItor;


#endif

