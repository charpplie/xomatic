
#ifndef __CSHADER_H__
#define __CSHADER_H__

#include <map>
#include "CShaderBin.h"
#include "ShaderSerialize.h"
#include "ShaderCache.h"

struct SRenderBuf;
class CRendElementBase;
struct SEmitter;
struct SParticleInfo;
struct SPartMoveStage;
struct SSunFlare;

#include <crc32.h>

Crc32Gen& GetCRC32Gen();

//===================================================================

#define MAX_ENVLIGHTCUBEMAPS 16
#define ENVLIGHTCUBEMAP_SIZE 16
#define MAX_ENVLIGHTCUBEMAPSCANDIST_UPDATE 16
#define MAX_ENVLIGHTCUBEMAPSCANDIST_THRESHOLD 2

#define MAX_ENVCUBEMAPS 16
#define MAX_ENVCUBEMAPSCANDIST_THRESHOLD 1

#define MAX_ENVTEXTURES 16
#define MAX_ENVTEXSCANDIST 0.1f

//===============================================================================

struct SMinMax
{
  int nMin, nMax;
};

struct SDevID
{
  string szName;
  int nVendorID;
  DynArray<SMinMax> DevMinMax;
};
struct SMSAAMode
{
  int nSamples;
  int nQuality;
  string szDescr;
};
struct SMSAAProfile
{
  string szName;
  SDevID *pDeviceGroup;
  DynArray<SMSAAMode> Modes;
};

//===============================================================================

struct SNameAlias
{
  CCryName m_Alias;
  CCryName m_Name;
  uint64 m_nGenMask;
  SNameAlias()
  {
    m_nGenMask = 0;
  }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_Alias);
		pSizer->AddObject(m_Name);
	}
};

//===================================================================

struct SMacroFX
{
  string m_szMacro;
  uint32 m_nMask;
};

//typedef stl::hash_map<string,SMacroFX,stl::hash_strcmp<string> > FXMacro;
//typedef std::map<string,SMacroFX> FXMacro;
typedef stl::hash_map<string,SMacroFX,stl::hash_strcmp<const char*> > FXMacro;

typedef FXMacro::iterator FXMacroItor;

//////////////////////////////////////////////////////////////////////////
// Helper class for shader parser, holds temporary strings vector etc...
//////////////////////////////////////////////////////////////////////////
struct CShaderParserHelper
{
	CShaderParserHelper()
	{
	}
	~CShaderParserHelper()
	{
	}

	char *GetTempStringArray( int nIndex,int nLen )
	{
		m_tempString.reserve(nLen+1);
		return (char*)&(m_tempStringArray[nIndex])[0];
	}

	std::vector<char> m_tempStringArray[32];
	std::vector<char> m_tempString;
};
extern CShaderParserHelper* g_pShaderParserHelper;

//==================================================================================

#define PD_INDEXED 1
#define PD_MERGED  4

struct SParamDB
{
  const char *szName;
  const char *szAliasName;
  ECGParam eParamType;
  uint32 nFlags;
  void (*ParserFunc)(const char *szScr, const char *szAnnotations, DynArray<STexSampler>* pSamplers, SCGParam *vpp, int nComp, CShader *ef);
  SParamDB()
  {
    szName = NULL;
    nFlags = 0;
    ParserFunc = NULL;
    eParamType = ECGP_Unknown;
  }
  SParamDB(const char *inName, ECGParam ePrmType, uint32 inFlags)
  {
    szName = inName;
    szAliasName = NULL;
    nFlags = inFlags;
    ParserFunc = NULL;
    eParamType = ePrmType;
  }
  SParamDB(const char *inName, ECGParam ePrmType, uint32 inFlags, void (*InParserFunc)(const char *szScr, const char *szAnnotations, DynArray<STexSampler>* pSamplers, SCGParam *vpp, int nComp, CShader *ef))
  {
    szName = inName;
    szAliasName = NULL;
    nFlags = inFlags;
    ParserFunc = InParserFunc;
    eParamType = ePrmType;
  }
};

enum EShaderCacheMode
{
  eSC_Normal = 0,
  eSC_BuildPerLevel = 1,
  eSC_BuildGlobal = 2,
  eSC_BuildGlobalList = 3,
  eSC_Preactivate = 4,
};

struct SCacheLevelData
{
  byte *pData;
  CCryNameCRC Name;
  int16 nSizeDecompressedShader;
  int16 nSizeCompressedShader;
  int32 nOffset;
  uint32 nDeviceID;
};
struct ShadersLevel
{
  string szPath;
  string szContent;
};
struct SCacheLevel
{
  SShaderCache *pCache;
  uint32 nRef;
  std::vector<SCacheLevelData> Data;
  SCacheLevel()
  {
    pCache = NULL;
    nRef = 0;
  }
};
//////////////////////////////////////////////////////////////////////////
class CShaderMan : public CShaderSerialize
{
friend class CShader;
friend class CParserBin;

private:
  int mfReadTexSequence(TArray<CTexture *>& txs, const char *name, byte eTT, int Flags, float fAmount1=-1.0f, float fAmount2=-1.0f);

  CShader *mfNewShader(const char *szName);

  bool mfCompileShaderGen(SShaderGen *shg, char *scr);
  SShaderGenBit *mfCompileShaderGenProperty(char *scr);

  CTexture *mfTryToLoadTexture(const char *nameTex, int Flags, byte eTT, float fAmount1=-1.0f, float fAmount2=-1.0f);
  CTexture *mfLoadResourceTexture(const char *nameTex, const char *path, int Flags, byte eTT, SEfResTexture *Tex, float fAmount1=-1.0f, float fAmount2=-1.0f);
  void mfCheckShaderResTextures(TArray<SShaderPass> &Dst, CShader *ef, SRenderShaderResources *Res);
  void mfCheckShaderResTexturesHW(TArray<SShaderPass> &Dst, CShader *ef, SRenderShaderResources *Res);
  bool mfCheckAnimatedSequence(STexSampler *tl, CTexture *tx);
  CTexture *mfCheckTemplateTexName(const char *mapname, ETEX_Type eTT, short &nFlags);

  CShader *mfCompile(CShader *ef, char *scr);

  bool mfUpdateMergeStatus(SShaderTechnique *hs, std::vector<SCGParam> *p);  
  void mfRefreshResources(SRenderShaderResources *Res, const bool bLoadNormalAlpha );

  bool mfReloadShaderFile(const char *szName, int nFlags);
#ifndef XENON
	bool CheckAllFilesAreWritable( const char *szDir ) const;
#endif

  bool ParseFSAADevices(char *scr, SDevID *pDev);
  bool ParseFSAADevGroup(char *scr, SDevID *pDev);
  bool ParseFSAAMode(char *scr, SMSAAMode *pMSAA);
  bool ParseFSAAProfile(char *scr, SMSAAProfile *pMSAA);

public:
  char *m_pCurScript;
  CShaderManBin m_Bin;

  const char *mfTemplateTexIdToName(int Id);
  SShaderGenComb *mfGetShaderGenInfo(const char *nmFX);

  bool mfReloadAllShaders(int nFlags, uint32 nFlagsHW);
  bool mfReloadFile(const char *szPath, const char *szName, int nFlags);

  void ParseShaderProfiles();
  void ParseShaderProfile(char *scr, SShaderProfile *pr);

  SParamDB *mfGetShaderParamDB(const char *szSemantic);
  const char *mfGetShaderParamName(ECGParam ePR);
  bool mfParseParamComp(int comp, SCGParam *pCurParam, const char *szSemantic, char *params, const char *szAnnotations, DynArray<STexSampler>* pSamplers, CShader *ef, uint32 nParamFlags, EHWShaderClass eSHClass, bool bExpressionOperand);
  bool mfParseCGParam(char *scr, const char *szAnnotations, DynArray<SFXParam>& Params, DynArray<STexSampler>* pSamplers, CShader *ef, std::vector<SCGParam>* pParams, int nComps, uint32 nParamFlags, EHWShaderClass eSHClass, bool bExpressionOperand);
  bool mfParseFXParameter(DynArray<SFXParam>& Params, SFXParam *pr, DynArray<STexSampler>* pSamplers, const char *ParamName, CShader *ef, bool bInstParam, int nParams, std::vector<SCGParam>* pParams, EHWShaderClass eSHClass, bool bExpressionOperand);

  void ParseFSAAProfiles();

  void mfCheckObjectDependParams(std::vector<SCGParam>& PNoObj, std::vector<SCGParam>& PObj, EHWShaderClass eSH, CShader *pFXShader);
  
  void mfBeginFrame();

  void mfGetShaderListPath(stack_string& nameOut, int nType);

  CShaderGraph m_GR;

public:
  bool m_bInitialized;

  const char *m_ShadersPath;
  const char *m_ShadersCache;
  const char *m_ShadersFilter;
  const char *m_ShadersMergeCachePath;
  string m_szUserPath;

  int m_nFrameForceReload;

  char m_HWPath[128];
  
  CShader *m_pCurShader;
  static SResourceContainer *m_pContainer;  // List/Map of objects for shaders resource class

  std::vector<SNameAlias> m_AliasNames;
  std::vector<SNameAlias> m_CustomAliasNames;
  std::vector<string> m_ShaderNames;

  static CCryNameCRC s_cNameHEAD;

  static CShader *m_DefaultShader;
  static CShader *m_shPostEffects;      // engine specific post process effects
  static CShader *m_shPostDepthOfField; // depth of field
  static CShader *m_shPostMotionBlur; 
  static CShader *m_shPostSunShafts; 
	static CShader *m_sh3DHUD; 
  
	// Deferred rendering passes
	static CShader *m_shDeferredShading; 
	static CShader *m_ShaderDeferredCaustics;
  static CShader *m_ShaderDeferredRain;

#ifndef NULL_RENDERER
  static CShader *m_ShaderFPEmu;
  static CShader *m_ShaderFallback;
  static CShader *m_ShaderTreeSprites;
  static CShader *m_ShaderShadowBlur;
	static CShader *m_ShaderShadowMaskGen;
  static CShader *m_ShaderAmbientOcclusion;
	static CShader *m_ShaderScreenSpaceGI;
	static CShader *m_ShaderCloudShadowMaskGen;
	static CShader *m_shHDRPostProcess;  
  static CShader *m_shPostEffectsGame;  // game specific post process effects
  static CShader *m_ShaderDebug;
  static CShader *m_ShaderLightFlares;
  static SShaderItem m_ShaderLightStyles;
  static CShader *m_ShaderCommon;
  static CShader *m_ShaderOcclTest;	
	static CShader *m_ShaderDXTCompress;  
	static CShader *m_ShaderStereo;
#else
  static SShaderItem m_DefaultShaderItem;
#endif

  const SInputShaderResources *m_pCurInputResources;
  SShaderGen *m_pGlobalExt;

  Vec4 m_TempVecs[16];
  Vec4 m_RTRect;
  std::vector<SShaderGenComb> m_SGC;
  
  int m_nCombinationsProcess;
  int m_nCombinationsProcessOverall;
  int m_nCombinationsCompiled;
  int m_nCombinationsEmpty;

  EShaderCacheMode m_eCacheMode;

  bool m_bActivatePhase;
  char *m_szShaderPrecache;
  FXShaderCacheCombinations m_ShaderCacheCombinations[2];
  FILE *m_FPCacheCombinations[2];
  SShaderProfile m_ShaderProfiles[eST_Max];
  SShaderProfile m_ShaderFixedProfiles[eSQ_Max];

  DynArray<SDevID *> m_FSAADevGroups;
  DynArray<SMSAAProfile> m_FSAAModes;

	int m_bActivated;

	CShaderParserHelper m_shaderParserHelper;

  bool m_bReload;

  
  // Shared common global flags data

  // Map used for storing automatically-generated flags and mapping old shader names masks to generated ones
  //  map< shader flag names, mask > 
  typedef std::map< string , uint64 > MapNameFlags;
  typedef MapNameFlags::iterator MapNameFlagsItor;
  MapNameFlags m_pShaderCommonGlobalFlag; 

  MapNameFlags m_pSCGFlagLegacyFix; 
  uint64 m_nSGFlagsFix;

  // Map stored for convenience mapping betweens old flags and new ones
  //  map < shader name , map< shader flag names, mask > >
  typedef std::map< string , MapNameFlags * > ShaderMapNameFlags;
  typedef ShaderMapNameFlags::iterator ShaderMapNameFlagsItor;
  ShaderMapNameFlags m_pShadersGlobalFlags; 

  SCGParamsPF m_PF;

  // Concatenated list of shader names using automatic masks generation
  string m_pShadersRemapList;

  // Helper functors for cleaning up

  struct SShaderMapNameFlagsContainerDelete
  {
    void operator()( ShaderMapNameFlags::value_type &pObj )
    {
      SAFE_DELETE( pObj.second );
    }
  };

public:
  CShaderMan()
  {
    m_bInitialized = false;
    m_DefaultShader = NULL;
    m_pGlobalExt = NULL;
		g_pShaderParserHelper = &m_shaderParserHelper;
    m_nCombinationsProcess = -1;
    m_nCombinationsProcessOverall = -1;
    m_nCombinationsCompiled = -1;
    m_nCombinationsEmpty = -1;
    m_szShaderPrecache = NULL;
		memset(m_TempVecs,0,sizeof(Vec4)*16);
		memset(&m_RTRect,0,sizeof(Vec4));
    m_eCacheMode = eSC_Normal;
  }

  void ShutDown();
  void mfReleaseShaders();
  
  SShaderGen *mfCreateShaderGenInfo(const char *szName);
	void mfRemapShaderGenInfoBits( const char *szName, SShaderGen *pShGen);
	
  uint64 mfGetRemapedShaderMaskGen( const char *szName, uint64 nMaskGen = 0, bool bFixup = 0);
	string mfGetShaderBitNamesFromMaskGen( const char *szName, uint64 nMaskGen );

  bool mfUsesGlobalFlags( const char *szShaderName );
  const char *mfGetShaderBitNamesFromGlobalMaskGen(uint64 nMaskGen );
  uint64 mfGetShaderGlobalMaskGenFromString( const char *szShaderGen );         

  void mfInitGlobal(void);

  void mfInitCommonGlobalFlags(void);
  void mfInitCommonGlobalFlagsLegacyFix(void);
  bool mfRemapCommonGlobalFlagsWithLegacy(void);
  void mfCreateCommonGlobalFlags( const char *szName );
  void mfSaveCommonGlobalFlagsToDisk( const char *szName, uint32 nMaskCount );

  void mfInit(void);
  void mfSortResources();
  uint64 mfShaderNameForAlias(const char *nameAlias, char *nameEf, int nSize, uint64 nGenMask);
  SRenderShaderResources *mfCreateShaderResources(const SInputShaderResources *Res, bool bShare);
  SShaderItem mfShaderItemForName (const char *szName, bool bShare, int flags, const SInputShaderResources *Res=NULL, uint64 nMaskGen=0);
  CShader *mfForName (const char *name, int flags, const SRenderShaderResources *Res=NULL, uint64 nMaskGen=0);

  SFXParam *mfGetFXParameter(DynArray<SFXParam>& Params, const char *param);
  const char *mfParseFX_Parameter (const string& buf, EParamType eType, const char *szName);
  void    mfParseFX_Annotations_Script (char *buf, CShader *ef, std::vector<SFXStruct>& Structs, bool *bPublic, CCryName techStart[2]);
  void    mfParseFX_Annotations (char *buf, CShader *ef, std::vector<SFXStruct>& Structs, bool *bPublic, CCryName techStart[2]);
  void    mfParseFXTechnique_Annotations_Script (char *buf, CShader *ef, std::vector<SFXStruct>& Structs, SShaderTechnique *pShTech, bool *bPublic, DynArray<SShaderTechParseParams>& techParams);
  void    mfParseFXTechnique_Annotations (char *buf, CShader *ef, std::vector<SFXStruct>& Structs, SShaderTechnique *pShTech, bool *bPublic, DynArray<SShaderTechParseParams>& techParams);
  void    mfParseFXSampler_Annotations_Script (char *buf, CShader *ef, std::vector<SFXStruct>& Structs, STexSampler *pSamp);
  void    mfParseFXSampler_Annotations (char *buf, CShader *ef, std::vector<SFXStruct>& Structs, STexSampler *pSamp);
  void    mfParseFX_Global (SFXParam& pr, CShader *ef, std::vector<SFXStruct>& Structs, CCryName techStart[2]);
  bool    mfParseDummyFX_Global (std::vector<SFXStruct>& Structs, char *annot, CCryName techStart[2]);
  const string& mfParseFXTechnique_GenerateShaderScript (std::vector<SFXStruct>& Structs, FXMacro& Macros, DynArray<SFXParam>& Params, DynArray<SFXParam>& AffectedParams, const char *szEntryFunc, CShader *ef, EHWShaderClass eSHClass, const char *szShaderName, uint32& nAffectMask, const char *szType);
  bool    mfParseFXTechnique_MergeParameters (std::vector<SFXStruct>& Structs, DynArray<SFXParam>& Params, std::vector<int>& AffectedFunc, SFXStruct *pMainFunc, CShader *ef, EHWShaderClass eSHClass, const char *szShaderName, DynArray<SFXParam>& NewParams);
  CTexture *mfParseFXTechnique_LoadShaderTexture (STexSampler *smp, SShaderPass *pShPass, CShader *ef, int nIndex, byte ColorOp, byte AlphaOp, byte ColorArg, byte AlphaArg);
  bool    mfParseFXTechnique_LoadShader (const char *szShaderCom, SShaderPass *pShPass, CShader *ef, DynArray<STexSampler>& Samplers, std::vector<SFXStruct>& Structs, DynArray<SFXParam>& Params, FXMacro& Macros, EHWShaderClass eSHClass);
  bool    mfParseFXTechniquePass (char *buf, char *annotations, SShaderTechnique *pShTech, CShader *ef, DynArray<STexSampler>& Samplers, std::vector<SFXStruct>& Structs, DynArray<SFXParam>& Params);
  bool    mfParseFXTechnique_CustomRE (char *buf, const char *name, SShaderTechnique *pShTech, CShader *ef);
  SShaderTechnique *mfParseFXTechnique (char *buf, char *annotations, CShader *ef, DynArray<STexSampler>& Samplers, std::vector<SFXStruct>& Structs, DynArray<SFXParam>& Params, bool *bPublic, DynArray<SShaderTechParseParams>& techParams);
  bool    mfParseFXSampler(char *buf, char *name, char *annotations, CShader *ef, DynArray<STexSampler>& Samplers, std::vector<SFXStruct>& Structs);
  bool    mfParseLightStyle(CLightStyle *ls, char *buf);
  bool    mfParseFXLightStyle(char *buf, int nID, CShader *ef, std::vector<SFXStruct>& Structs);
  CShader *mfParseFX (char *buf, CShader *ef, CShader *efGen, uint64 nMaskGen);
  void    mfPostLoadFX(CShader *efT, DynArray<SShaderTechParseParams>& techParams, CCryName techStart[2]);
  bool     mfParseDummyFX(char *buf, std::vector<string>& ShaderNames, const char *szName);
  bool     mfAddFXShaderNames(const char *szName, std::vector<string>* ShaderNames, bool bUpdateCRC);
  bool     mfInitShadersDummy(bool bUpdateCRC);

  uint64   mfGetRTForName(char *buf);
  uint32     mfGetGLForName(char *buf, CShader *ef);

  void mfFillGenMacroses(SShaderGen *shG, TArray<char>& buf, uint64 nMaskGen);
  bool mfModifyGenFlags(CShader *efGen, const SRenderShaderResources *pRes, uint64& nMaskGen, uint64& nMaskGenHW);

  bool mfGatherShadersList(const char *szPath, bool bCheckIncludes, bool bUpdateCRC, std::vector<string> *Names);
  void mfGatherFilesList(const char *szPath, std::vector<CCryName>& Names, int nLevel, bool bUseFilter, bool bMaterial=false);
  int  mfInitShadersList(std::vector<string> *ShaderNames);
  void mfSetDefaults(void);
  void mfCloseShadersCache(int nID);
  void mfInitShadersCache(bool bForLevel, FXShaderCacheCombinations *Combinations=NULL, const char* pCombinations=NULL, int nType=0);
  void mfMergeShadersCombinations(FXShaderCacheCombinations *Combinations, int nType);
  void mfInsertNewCombination(uint64 nGL, uint64 nRT, uint32 nLT, uint32 nMD, uint32 nMDV, EHWShaderClass eCL, const char *name, int nID, string *Str=NULL, byte bStore=1);
	const char *mfGetShaderCompileFlags(EHWShaderClass eClass) const;

  void mfPreactivateShaders(FXShaderCacheCombinations *Combinations);
  bool mfReleasePreactivatedShaderData();
  bool mfPreactivateShaders2();
  bool mfPreactivate2(string szPath, bool bVS);

#if !defined(XENON) && !defined(PS3) && !defined(NULL_RENDERER)
  void AddCombination(SCacheCombination& cmb,  FXShaderCacheCombinations& CmbsMap, CHWShader *pHWS);
  void AddGLCombinations(CShader *pSH, std::vector<SCacheCombination>& CmbsGL);
  void AddLTCombinations(SCacheCombination& cmb, FXShaderCacheCombinations& CmbsMap, CHWShader *pHWS);
  void AddRTCombinations(FXShaderCacheCombinations& CmbsMap, CHWShader *pHWS, CShader *pSH, FXShaderCacheCombinations *Combinations);
  void AddGLCombination(FXShaderCacheCombinations& CmbsMap, SCacheCombination& cc);
  void mfPrecacheShaders(bool bListOnly, bool bStatsOnly);
  void mfGetLevelShadersList(std::vector<ShadersLevel>& ShaderList, const char *szDir);
  void mfStoreLevelCache(const char *szName, const ShadersLevel& sl, std::vector<SCacheLevel>& Caches);
  SCacheLevel *mfOpenLevelCache(const char *szCache, const char *szName, SCacheCombination *cmb, std::vector<SCacheLevel>& Caches);
  void _PrecacheShadersLevels();
  void mfPrecacheShadersLevels();
	void mfPakPerLevelShaderCache();
  void _PrecacheShaderList(bool bGLOnly, bool bStatsOnly);
  void mfOptimiseShaders(const char *szFolder, bool bForce);
  void mfMergeShaders();
  void _MergeShaders();
  void mfAddRTCombinations(FXShaderCacheCombinations& CmbsMapSrc, FXShaderCacheCombinations& CmbsMapDst, CHWShader *pSH, bool bListOnly);
  void mfAddRTCombination_r(int nComb, FXShaderCacheCombinations& CmbsMapDst, SCacheCombination *cmb, CHWShader *pSH, bool bAutoPrecache);
  void mfAddLTCombinations(SCacheCombination *cmb, FXShaderCacheCombinations& CmbsMapDst);
  void mfAddLTCombination(SCacheCombination *cmb, FXShaderCacheCombinations& CmbsMapDst, DWORD dwL);
#endif

#ifdef WIN64
#pragma warning( push )							//AMD Port
#pragma warning( disable : 4267 )				// conversion from 'size_t' to 'XXX', possible loss of data
#endif

  int Size()
  {
    int nSize = sizeof(*this);

    nSize += m_AliasNames.size()*sizeof(SNameAlias);
    nSize += m_SGC.capacity();
    nSize += m_Bin.Size();

    return nSize;
  }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_Bin);
		pSizer->AddObject(m_AliasNames);		
		pSizer->AddObject(m_SGC);
		pSizer->AddObject(m_CustomAliasNames);	
		pSizer->AddObject(m_ShaderNames);		
		pSizer->AddObject(m_ShaderCacheCombinations[0]);
		pSizer->AddObject(m_ShaderCacheCombinations[1]);
	}

  static float EvalWaveForm(SWaveForm *wf);
  static float EvalWaveForm(SWaveForm2 *wf);
  static float EvalWaveForm2(SWaveForm *wf, float frac);
};

#ifdef WIN64
#pragma warning( pop )							//AMD Port
#endif

//=====================================================================

#endif  // __CSHADER_H__
