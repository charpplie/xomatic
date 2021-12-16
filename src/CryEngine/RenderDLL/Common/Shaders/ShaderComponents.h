/*=============================================================================
  ShaderComponents.h : FX Shaders semantic components declarations.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#ifndef __SHADERCOMPONENTS_H__
#define __SHADERCOMPONENTS_H__

#include "../Defs.h"

#define PF_LOCAL      1
#define PF_CANMERGED  2
#define PF_DONTALLOW_DYNMERGE 4
#define PF_INTEGER    8
#define PF_BOOL       0x10
#define PF_POSITION   0x20
#define PF_MATRIX     0x40
#define PF_SCALAR     0x80
#define PF_TWEAKABLE_0 0x100
#define PF_TWEAKABLE_1 0x200
#define PF_TWEAKABLE_2 0x400
#define PF_TWEAKABLE_3 0x800
#define PF_TWEAKABLE_MASK 0xf00
#define PF_INSTANCE    0x1000
#define PF_AUTOMERGED  0x10000
#define PF_ALWAYS      0x20000
#define PF_SINGLE_COMP 0x40000
#define PF_GLOBAL      0x80000
#define PF_CUSTOM_BINDED 0x100000
#define PF_MATERIAL      0x200000
#define PF_LIGHT         0x400000
#define PF_SHADOWGEN     0x800000

enum ECGParam
{
  ECGP_Unknown,
  
  ECGP_Matr_SI_Obj,
  ECGP_SI_AmbientOpacity,
  ECGP_SI_BendInfo,
  ECGP_SI_ObjectAmbColComp,
  ECGP_PI_AlphaTest,
  ECGP_Matr_PI_Obj_T,
  ECGP_PI_MotionBlurData,
  ECGP_PI_CloakParams,
  ECGP_Matr_PI_ViewProj,
  ECGP_Matr_PI_Composite,
  ECGP_Matr_PI_ObjOrigComposite,
  ECGP_PI_OSCameraPos,
  ECGP_PI_Ambient,
  ECGP_PI_VisionParams,
	ECGP_PI_EffectLayerParams,
  ECGP_PI_MaterialLayersParams,
  ECGP_PI_FrozenLayerParams,
  ECGP_PI_AvgFogVolumeContrib,
  ECGP_PI_NumInstructions,
  ECGP_PI_ObjColor,
  ECGP_PI_Wind,
  ECGP_PI_TextureTileSize,
  ECGP_PI_HMAGradients,
  ECGP_Matr_PI_OceanMat,
  ECGP_PB_AmbientOpacity,

  ECGP_PB_Scalar,
  ECGP_PM_Tweakable,
  ECGP_Matr_PF_ViewProjMatrix,
  ECGP_Matr_PF_ViewProjZeroMatrix,
  ECGP_Matr_PB_ViewProjMatrix_IT,
  ECGP_Matr_PB_ViewProjMatrix_I,
  ECGP_Matr_PB_ProjMatrix,
  ECGP_Matr_PB_UnProjMatrix,
  
  ECGP_Matr_PB_View,
  ECGP_Matr_PB_View_I,
  ECGP_Matr_PB_View_T,
  ECGP_Matr_PB_View_IT,

  ECGP_Matr_PB_Camera,
  ECGP_Matr_PB_Camera_I,
  ECGP_Matr_PB_Camera_T,
  ECGP_Matr_PB_Camera_IT,

  ECGP_Matr_PB_Temp4_0,
  ECGP_Matr_PB_Temp4_1,
  ECGP_Matr_PB_Temp4_2,
  ECGP_Matr_PB_Temp4_3,
  ECGP_Matr_PB_TerrainBase,
  ECGP_Matr_PB_TerrainLayerGen,
  ECGP_Matr_PB_VoxTerrainAtlasInfo,
  ECGP_Matr_PB_LightMatrix,
  ECGP_Matr_PI_TexMatrix,
  ECGP_Matr_PI_TCMMatrix,
  ECGP_Matr_PI_TCGMatrix,

  ECGP_PL_LightsNum,
  ECGP_PL_LightsPos,
  ECGP_PB_RotGridScreenOff,
  ECGP_PL_ShadowMasks,
  ECGP_PB_DiffuseMulti,
  ECGP_PB_SpecularMulti,
  ECGP_PL_LDiffuseColors,
  ECGP_PL_LSpecularColors,
  ECGP_PM_MatDiffuseColor,
  ECGP_PM_MatSpecularColor,
  ECGP_PM_MatEmissiveColor,
	ECGP_PB_OutdoorAOParams,
  ECGP_PB_GlowParams,
  ECGP_PB_FurParams,
  ECGP_PB_ResourcesOpacity,
  ECGP_PB_RandomParams,
  ECGP_PB_IrregKernel,
  ECGP_PB_RegularKernel,
  ECGP_PB_DeformWaveX,
  ECGP_PB_DeformWaveY,
	ECGP_PB_DeformFrequencies,
	ECGP_PB_DeformAmplitudes,
	ECGP_PB_DeformLevels,
	ECGP_PB_DeformVertexPhases,
  ECGP_PB_DeformBend,
  ECGP_PB_DeformNoiseInfo,
  ECGP_PB_TFactor,
  ECGP_PB_AlphaTest,
  ECGP_PB_TempData,
  ECGP_PB_DecalZFightingRemedy,
  ECGP_PB_VolumetricFogParams,
  ECGP_PB_VolumetricFogRampParams,
  ECGP_PB_VolumetricFogColor,
  ECGP_PB_FogColGradColBase,
  ECGP_PB_FogColGradColDelta,
  ECGP_PB_CameraFront,
  ECGP_PB_CameraRight,
  ECGP_PB_CameraUp,
  ECGP_PB_RTRect,
  ECGP_PB_SkyLightHazeColorPartialMieInScatter,
  ECGP_PB_SkyLightHazeColorPartialRayleighInScatter,
  ECGP_PB_SkyLightSunDirection,
  ECGP_PB_SkyLightPhaseFunctionConstants,
  ECGP_PB_LightInfoTC,  
  ECGP_PB_LightningPos,
  ECGP_PB_LightningColSize,
  ECGP_PB_FromRE,
  ECGP_PB_GlobalShaderFlag,
  ECGP_PB_RuntimeShaderFlag,
	ECGP_PB_ObjVal,
  ECGP_PB_LightsNum,
  ECGP_PB_WaterLevel,
  ECGP_PB_HDRDynamicMultiplier,
  ECGP_PB_CausticsParams,
  ECGP_PB_CausticsSmoothSunDirection,
	ECGP_PF_SunDirection,
  ECGP_PF_FogColor,
  ECGP_PF_SunColor,
  ECGP_PF_SkyColor,
  ECGP_PF_Time,
  ECGP_PF_CameraPos,
  ECGP_PF_ScreenSize,
  ECGP_PF_NearFarDist,
  ECGP_PF_ProjRatio,
  ECGP_PF_DepthFactor,
	
	ECGP_Matr_PB_SFCompMat,
	ECGP_Matr_PB_SFTexGenMat0,
	ECGP_Matr_PB_SFTexGenMat1,
	ECGP_PB_SFBitmapColorTransform,
	ECGP_PB_SFSRGBFixup,

	ECGP_PB_CloudShadingColorSun,
	ECGP_PB_CloudShadingColorSky,

	ECGP_PB_ResInfoDiffuse,
	ECGP_PB_ResInfoBump,
	ECGP_PB_TexelDensityParam,
	ECGP_PB_TexelDensityColor,
	ECGP_PB_TexelsPerMeterInfo,

  ECGP_SG_FrustrumInfo,
  ECGP_Matr_SG_ShadowProj_0,
  ECGP_Matr_SG_ShadowProj_1,
  ECGP_Matr_SG_ShadowProj_2,
  ECGP_Matr_SG_ShadowProj_3,

	ECGP_PB_ColorCharts_WidthFixPS3,

  ECGP_ParamCount,
};

// Per frame - harcoded - commonly used data
struct SCGParamsPF
{
public:
  int nFrameID;

  Matrix44 pProjMatrix; //ECGP_Matr_PB_ProjMatrix
  Matrix44 pUnProjMatrix; //ECGP_Matr_PB_UnProjMatrix
  Matrix44 pMatrixComposite; //ECGP_Matr_PI_Composite

  Vec3 vWaterLevel; // ECGP_PB_WaterLevel *
  float fHDRDynamicMultiplier; // ECGP_PB_HDRDynamicMultiplier *

  Vec4 pFogColGradColBase; // ECGP_PB_FogColGradColBase *
  Vec4 pFogColGradColDelta; // ECGP_PB_FogColGradColDelta *

  Vec4 pVolumetricFogParams;  // ECGP_PB_VolumetricFogParams *
  Vec4 pVolumetricFogRampParams;  // ECGP_PB_VolumetricFogRampParams *
  Vec3 pVolumetricFogColor; // ECGP_PB_VolumetricFogColor *
  Vec3 pSkyLightHazeColorPartialMieInScatter; //ECGP_PB_SkyLightHazeColorPartialMieInScatter * 
  Vec4 pSkyLightHazeColorPartialRayleighInScatter; //ECGP_PB_SkyLightHazeColorPartialRayleighInScatter *
  SSkyLightRenderParams *pSkyLightRenderParams; // *

  Vec3 pCameraFront; // ECGP_PB_CameraFront
  Vec3 pCameraRight; // ECGP_PB_CameraRight
  Vec3 pCameraUp; // ECGP_PB_CameraUp
  Vec3 pCameraPos; //ECGP_PF_CameraPos

  Vec4 pScreenSize; //ECGP_PF_ScreenSize
  Vec4 pNearFarDist; //ECGP_PF_NearFarDist

  Vec3 pDecalZFightingRemedy; //ECGP_PB_DecalZFightingRemedy

  Vec4 pLightningColSize; // ECGP_PB_LightningColSize
  Vec3 pLightningPos; //ECGP_PB_LightningPos

  Vec3 pCausticsParams; //ECGP_PB_CausticsParams * 
  Vec3 pSunColor; //ECGP_PF_SunColor * 
  Vec3 pSkyColor; //ECGP_PF_SkyColor *
  Vec3 pSunDirection; //ECGP_PF_SunDirection *

  Vec3 pCloudShadingColorSun; //ECGP_PB_CloudShadingColorSun *
  Vec3 pCloudShadingColorSky; //ECGP_PB_CloudShadingColorSky *

  Vec3 vCausticsCurrSunDir;
  int nCausticsFrameID;

  float e_vegetation_alpha_blend_near;

  bool bPE_NVActive;
};

enum EOperation
{
  eOp_Unknown,
  eOp_Add,
  eOp_Sub,
  eOp_Div,
  eOp_Mul,
  eOp_Log,
};

struct SCGBind
{
  CCryName m_Name;
  unsigned int m_Flags;
#if defined (DIRECT3D9) || defined (OPENGL) || defined (NULL_RENDERER)
  int m_dwBind;
#elif defined (DIRECT3D10)
  short m_dwBind;
  short m_dwCBufSlot;
#elif defined(DEVIRT_GEN_MANGLE_NAMES)
	int m_dwBind; // dummy to allow mangle name generation(since there no DIRECT3D macros are defined
#endif
  int m_nParameters;
  SCGBind()
  {
    m_nParameters = 1;
    m_dwBind = -2;
#if defined (DIRECT3D10)
    m_dwCBufSlot = 0;
#endif
    m_Flags = 0;
  }
  SCGBind (const SCGBind& sb)
  {
    m_Name = sb.m_Name;
    m_dwBind = sb.m_dwBind;
#if defined (DIRECT3D10)
    m_dwCBufSlot = sb.m_dwCBufSlot;
#endif
    m_nParameters = sb.m_nParameters;
    m_Flags = sb.m_Flags;
  }
  SCGBind& operator = (const SCGBind& sb)
  {
    this->~SCGBind();
    new(this) SCGBind(sb);
    return *this;
  }
  int Size()
  {
    return sizeof(SCGBind);
  }
	void GetMemoryUsage(ICrySizer *pSizer) const{}
};

struct SParamData
{
  CCryName m_CompNames[4];
  union UData
  {
    uint64 nData64[4];
    uint32 nData32[4];
    float fData[4];
  }d;
  SParamData()
  {
		memset(&d, 0, sizeof(UData));
  }
  ~SParamData();
  SParamData(const SParamData& sp);
  SParamData& operator = (const SParamData& sp)
  {
    this->~SParamData();
    new(this) SParamData(sp);
    return *this;
  }
  unsigned Size() { return sizeof(SParamData); }
	void GetMemoryUsage(ICrySizer *pSizer)const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

struct SCGLiteral
{
  int m_nIndex;
  //Vec4 m_vVec;
  unsigned Size() { return sizeof(SCGLiteral); }
};

struct SCGParam : SCGBind 
{
  ECGParam m_eCGParamType;
  SParamData *m_pData;
  UINT_PTR m_nID;
  SCGParam()
  {
    m_eCGParamType = ECGP_Unknown;
    m_pData = NULL;
    m_nID = 0;
  }
  ~SCGParam()
  {
    SAFE_DELETE(m_pData);
  }
  SCGParam(const SCGParam& sp) : SCGBind(sp)
  {
    m_eCGParamType = sp.m_eCGParamType;
    m_nID = sp.m_nID;
    if (sp.m_pData)
    {
      m_pData = new SParamData;
      *m_pData = *sp.m_pData;
    }
    else
      m_pData = NULL;
  }
  SCGParam& operator = (const SCGParam& sp)
  {
    this->~SCGParam();
    new(this) SCGParam(sp);
    return *this;
  }
  bool operator != (const SCGParam& sp)
  {
    if (sp.m_dwBind == m_dwBind &&
        sp.m_Name == m_Name &&
        sp.m_nID == m_nID &&
        sp.m_nParameters == m_nParameters &&
        sp.m_eCGParamType == m_eCGParamType &&
#if defined(DIRECT3D10)
        sp.m_dwCBufSlot == m_dwCBufSlot &&
#endif
        sp.m_Flags == m_Flags &&
        !sp.m_pData && !m_pData)
    {
      return false;
    }
    return true;
  }
#ifndef USE_PER_MATERIAL_PARAMS
  bool GetTweakable(float *pData, int nComp) const;
#endif
  const CCryName GetParamCompName(int nComp) const
  {
    if (!m_pData)
      return CCryName("None");
    return m_pData->m_CompNames[nComp];
  }

  int Size()
  {
    int nSize = sizeof(SCGParam);
    if (m_pData)
      nSize += m_pData->Size();
    return nSize;
  }

	void GetMemoryUsage(ICrySizer *pSizer)const 
	{
		pSizer->AddObject(m_pData);
	}
};

#endif

