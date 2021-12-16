//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
//	
//	File:Renderer.cpp
//  Description: Abstract renderer API
//
//	History:
//	-Feb 05,2001:Originally Created by Marco Corbetta
//	-: taken over by Andrey Honich
//.
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Shadow_Renderer.h"
#include "IStatObj.h"
#include "I3DEngine.h"
#include "IIndexedMesh.h"
#include "CREParticle.h"
#include "BitFiddling.h"															// IntegerLog2()	
#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
#include "CREParticleGPU.h"
#endif
#include "ImageExtensionHelper.h"											// CImageExtensionHelper
#if !defined(EXCLUDE_SQUISH_SDK)
#include "Textures/Image/Squish/squish.h"							// CompressImage
#endif
#include "ResourceCompilerHelper.h"                   // CResourceCompilerHelper

#include "Textures/Image/CImage.h"
#include "../CryCommon/auto_vector.h"
#include "PostProcess/PostEffects.h"

#ifdef WIN64
	#pragma warning(disable: 4244)
#endif

#if defined(LINUX)
#include "ILog.h"
#endif





#include "Textures/Image/CompressedImage.h"

#ifndef NULL_RENDERER
# ifdef WIN32
#	 pragma message (">>> include lib: nvDXTlib")
//#	  ifdef WIN64
//#		  pragma comment ( lib, "nvDXTlibMTDLL.vc9.x64.lib" )
//#	  else // WIN64
//#		  pragma comment ( lib, "nvDXTlibMTDLL.vc9.lib" )
//#	  endif // WIN64

//#		ifdef WIN64
//#			pragma comment ( lib, "nvDXTLibrary.lib")
//#		else // WIN64
//#			pragma comment ( lib, "nvDXTLibrary.lib")
//#		endif // WIN64

#		ifdef WIN64
#			pragma comment ( lib, LIBPATH_FIXUP("../../../SDKs/nvDXTLibrary/x64/Release/nvDXTLibrary.lib") )
#		else // WIN64
#			pragma comment ( lib, LIBPATH_FIXUP("../../../SDKs/nvDXTLibrary/Win32/Release/nvDXTLibrary.lib") )
#		endif // WIN64

# endif // WIN32
#endif // NULL_RENDERER
//
//////////////////////////////////////////////////////////////////////////
// Globals.
//////////////////////////////////////////////////////////////////////////
CRenderer *gRenDev = NULL;

#define RENDERER_DEFAULT_FONT "Fonts/default.xml"

#define EYEADAPTIONBASEDEFAULT		0.25f					// only needed for Crysis

#if defined(_LIB)
	extern int g_CpuFlags;
#else
	int g_CpuFlags;
#endif

#ifndef _XBOX
#include <CrtDebugStats.h>
#endif

using namespace NPostEffects;

//////////////////////////////////////////////////////////////////////////
// Pool allocators.
//////////////////////////////////////////////////////////////////////////
SDynTexture_PoolAlloc* g_pSDynTexture_PoolAlloc = 0;
//////////////////////////////////////////////////////////////////////////

// per-frame profilers: collect the information for each frame for 
// displaying statistics at the beginning of each frame
//#define PROFILER(ID,NAME) DEFINE_FRAME_PROFILER(ID,NAME)
//#include "FrameProfilers-list.h"
//#undef PROFILER


///

/// Used by console auto completion.
struct STextureNameAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const
	{
		SResourceContainer* pRC = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
		if(!pRC)
			return 0;
		return pRC->m_RMap.size();
	}
	virtual const char* GetValue( int nIndex ) const
	{
		SResourceContainer* pRC = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
		if(!pRC || pRC->m_RMap.empty())
			return "";
		nIndex %= pRC->m_RMap.size();
		ResourcesMap::const_iterator it = pRC->m_RMap.begin();
		std::advance(it, nIndex);
		return it->first.c_str();
	}
} g_TextureNameAutoComplete;

// Common render console variables
int CRenderer::CV_r_vsync;
int CRenderer::CV_r_stats;
int CRenderer::CV_r_log;
int CRenderer::CV_r_logTexStreaming;
int CRenderer::CV_r_logShaders;
int CRenderer::CV_r_logVBuffers;
int CRenderer::CV_r_logVidMem;
int CRenderer::CV_r_flush;
int CRenderer::CV_r_predicatedtiling;
int CRenderer::CV_r_testSplitScreen;
int CRenderer::CV_r_SplitScreenActive;

int CRenderer::CV_r_ambientPartialStencilRefresh;
int CRenderer::CV_r_ambientIndexedDraw;

int CRenderer::CV_r_fsaa;
int CRenderer::CV_r_fsaa_samples;
int CRenderer::CV_r_fsaa_quality;

int CRenderer::CV_r_atoc;

int CRenderer::CV_r_multithreaded;
int CRenderer::CV_r_multigpu;
int CRenderer::CV_r_validateDraw;
int CRenderer::CV_r_profileGPU;

int CRenderer::CV_r_CBStatic;
int CRenderer::CV_r_CBStaticDebug;

int CRenderer::CV_r_texturesstreaming;
int CRenderer::CV_r_texturesstreamingdebug;
int CRenderer::CV_r_texturesstreamingnoupload;
int CRenderer::CV_r_texturesstreamingonlyvideo;
int CRenderer::CV_r_texturesstreamingsync;
int CRenderer::CV_r_texturesstreampoolsize;
float CRenderer::CV_r_texturesstreamingmaxasync;
int CRenderer::CV_r_TexturesStreamingMipBias;
int CRenderer::CV_r_texturesfilteringquality;

float CRenderer::CV_r_TextureLodDistanceRatio;
int CRenderer::CV_r_TextureLodMaxLod;

int CRenderer::CV_r_dyntexmaxsize;
int CRenderer::CV_r_dyntexatlascloudsmaxsize;
int CRenderer::CV_r_dyntexatlasspritesmaxsize;
int CRenderer::CV_r_dyntexatlasvoxterrainsize;
int CRenderer::CV_r_dyntexatlasdyntexsrcsize;

int CRenderer::CV_r_texpostponeloading;
int CRenderer::CV_r_texpreallocateatlases;
//int CRenderer::CV_r_texatlassize;
int CRenderer::CV_r_texmaxanisotropy;
int CRenderer::CV_r_texmaxsize;
int CRenderer::CV_r_texminsize;
int CRenderer::CV_r_texbumpresolution;
int CRenderer::CV_r_texresolution;
int CRenderer::CV_r_texlmresolution;
int CRenderer::CV_r_texskyresolution;
int CRenderer::CV_r_texnormalmaptype;
int CRenderer::CV_r_texhwmipsgeneration;
//int CRenderer::CV_r_texhwdxtcompression;
int CRenderer::CV_r_texgrid;
int CRenderer::CV_r_texlog;
int CRenderer::CV_r_texnoload;

int CRenderer::CV_r_debugrendermode;
int CRenderer::CV_r_debugrefraction;
float CRenderer::CV_r_DebugVoxTerrainX;
float CRenderer::CV_r_DebugVoxTerrainY;

int CRenderer::CV_r_deferredshading;
int CRenderer::CV_r_deferredshadingLightVolumes;
int CRenderer::CV_r_deferredshadingstencilprepass;
int CRenderer::CV_r_deferredshadingscissor;
int CRenderer::CV_r_deferredshadingdepthboundstest;
int CRenderer::CV_r_deferredshadingdebug;
float CRenderer::CV_r_deferredshadinglightlodratio;
int CRenderer::CV_r_deferredshadingcubemaps;
int CRenderer::CV_r_deferredshadingheightbasedambient;



int CRenderer::CV_r_deferredshadinginterleavedacc;

int CRenderer::CV_r_irradiancevolumes;

int CRenderer::CV_r_HDRRendering;
int CRenderer::CV_r_HDRDebug;
float CRenderer::CV_r_HDRLevel;
float CRenderer::CV_r_HDROffset;
float CRenderer::CV_r_HDRBrightOffset;
float CRenderer::CV_r_HDRBrightThreshold;
float CRenderer::CV_r_HDRBrightLevel;
float CRenderer::CV_r_HDRBlueShift;
float CRenderer::CV_r_HDRContrast;
float CRenderer::CV_r_HDRContrastLuminanceBlend;
int CRenderer::CV_r_HDRVignetting;
int CRenderer::CV_r_HDRTexFormat;
int CRenderer::CV_r_HDRForceUpdateTextures;
int CRenderer::CV_r_HDRRangeAdapt;
float CRenderer::CV_r_HDRRangeAdaptMax;
float CRenderer::CV_r_HDRRangeAdaptMaxRange;
float CRenderer::CV_r_HDRRangeAdaptLBufferMaxRange;
float CRenderer::CV_r_HDRRangeAdaptLBufferMax;		
float CRenderer::CV_r_HDREyeAdaptationFactor;
float CRenderer::CV_r_HDREyeAdaptationBase;
float CRenderer::CV_r_HDREyeAdaptationSpeed;
float CRenderer::CV_r_HDRRangeAdaptationSpeed;

int CRenderer::CV_r_geominstancing;
int CRenderer::CV_r_geominstancingthreshold;
int CRenderer::m_iGeomInstancingThreshold=0;		// 0 means not set yet

int CRenderer::CV_r_beams;
int CRenderer::CV_r_beamssoftclip;
int CRenderer::CV_r_beamshelpers;
int CRenderer::CV_r_beamsmaxslices;
float CRenderer::CV_r_beamsdistfactor;

int CRenderer::CV_r_DebugLightVolumes;

int CRenderer::CV_r_UseShadowsPool;

float CRenderer::CV_r_ShadowsSlopeScaleBias;
float CRenderer::CV_r_ShadowsBias;

int CRenderer::CV_r_ShadowGenMode;
int CRenderer::CV_capture_misc_render_buffers;

int CRenderer::CV_r_ShadowsUseClipVolume;
int CRenderer::CV_r_shadowblur;
int CRenderer::CV_r_shadowtexformat;
int CRenderer::CV_r_ShadowsMaskResolution;
int CRenderer::CV_r_ShadowsMaskDownScale;
int CRenderer::CV_r_ShadowsDeferredMode;
int CRenderer::CV_r_ShadowsStencilPrePass;
int CRenderer::CV_r_ShadowsDepthBoundNV;
int CRenderer::CV_r_ShadowsForwardPass;
float CRenderer::CV_r_shadowbluriness;
float CRenderer::CV_r_shadow_jittering;
float CRenderer::CV_r_VarianceShadowMapBlurAmount;
int CRenderer::CV_r_ShadowsGridAligned;
int	CRenderer::CV_r_ShadowGenGS;
int	CRenderer::CV_r_ShadowPass;
int	CRenderer::CV_r_ShadowGen;

float	CRenderer::CV_r_RenderMeshHashGridUnitSize;
int	CRenderer::CV_r_SSAO;
int	CRenderer::CV_r_SSAO_normals;
int	CRenderer::CV_r_SSAO_downscale;
int	CRenderer::CV_r_SSAO_downscale_ztarget;
int	CRenderer::CV_r_SSAO_downscale_result_mask;
float CRenderer::CV_r_SSAO_amount;
float CRenderer::CV_r_SSAO_darkening;
int	CRenderer::CV_r_SSAO_quality;
float CRenderer::CV_r_SSAO_radius;

int CRenderer::CV_r_SSGI;
float CRenderer::CV_r_SSGI_amount;
float CRenderer::CV_r_SSGI_radius;
int CRenderer::CV_r_SSGI_quality;
int CRenderer::CV_r_SSGI_blur;

int	CRenderer::CV_r_TerrainAO;
int	CRenderer::CV_r_TerrainAO_FadeDist;

int CRenderer::CV_r_noloadtextures;
int CRenderer::CV_r_debuglights;
int CRenderer::CV_r_lightssinglepass;

int CRenderer::CV_r_impostersdraw;
float CRenderer::CV_r_imposterratio;
int CRenderer::CV_r_impostersupdateperframe;

int CRenderer::CV_r_shaderslazyunload;
int CRenderer::CV_r_shadersuserfolder;
int CRenderer::CV_r_shadersdebug;
int CRenderer::CV_r_ShaderLod;
#if !defined (XENON) && !defined(PS3)
int CRenderer::CV_r_shadersxenon;
int CRenderer::CV_r_shadersps3;
int CRenderer::CV_r_shadersdx9;
int CRenderer::CV_r_shadersdx11;
#endif
int CRenderer::CV_r_shadersignoreincludeschanging;
int CRenderer::CV_r_shaderspreactivate;
int CRenderer::CV_r_shadersintcompiler;
int CRenderer::CV_r_shadersnocompile;
int CRenderer::CV_r_shadersnosources;
int CRenderer::CV_r_shadersremotecompiler;
int CRenderer::CV_r_shadersasynccompiling;
int CRenderer::CV_r_shadersasyncmaxthreads;
int CRenderer::CV_r_shaderscacheoptimiselog;
int CRenderer::CV_r_shadersprecachealllights;
int CRenderer::CV_r_shadersdynamicbranching;

int CRenderer::CV_r_optimisedlightsetup;

int CRenderer::CV_r_meshprecache;












int CRenderer::CV_r_ZPrePass;
float CRenderer::CV_r_ZPrePassRadiusThreshold;
float CRenderer::CV_r_ZPrePassDistanceThreshold;

int CRenderer::CV_r_usezpass;
int CRenderer::CV_r_usealphablend;
int CRenderer::CV_r_useedgeaa;
int CRenderer::CV_r_usehwskinning;
int CRenderer::CV_r_usemateriallayers;
int CRenderer::CV_r_usesoftparticles;
int CRenderer::CV_r_usewaterparticles;
int CRenderer::CV_r_useparticles_refraction;
int CRenderer::CV_r_useparticles_glow;
int CRenderer::CV_r_usepom;
int CRenderer::CV_r_PostMSAA;
int CRenderer::CV_r_PostStereo;

int CRenderer::CV_r_hair_sorting_quality;

Vec2 CRenderer::s_overscanBorders(0, 0);

int CRenderer::CV_r_motionblur;
int CRenderer::s_MotionBlurMode;				// potentially adjusted internal state of r_motionblur
float CRenderer::CV_r_motionblur_shutterspeed;
int CRenderer::CV_r_motionblurframetimescale;
int CRenderer::CV_r_motionblurdynamicquality;
float CRenderer::CV_r_motionblurdynamicquality_rotationacc_stiffness;  
float CRenderer::CV_r_motionblurdynamicquality_rotationthreshold;  
float CRenderer::CV_r_motionblurdynamicquality_translationthreshold;  

int CRenderer::CV_r_debug_extra_scenetarget_fsaa;

float CRenderer::CV_r_cloak_light_scale;

int CRenderer::CV_r_customvisions;
int CRenderer::CV_r_DebugLayerEffect; 

int CRenderer::CV_r_rain;
float CRenderer::CV_r_rainamount;

    
int CRenderer::CV_r_dof;
int CRenderer::CV_r_dof_bokeh;
int CRenderer::CV_r_dof_stencil_prepass;

int CRenderer::CV_r_measureoverdraw;
int CRenderer::CV_r_printmemoryleaks;
int CRenderer::CV_r_releaseallresourcesonexit;

int   CRenderer::CV_r_rc_autoinvoke;  

int CRenderer::CV_r_glow;
int CRenderer::CV_r_glowanamorphicflares;

int CRenderer::CV_r_refraction;
int CRenderer::CV_r_sunshafts; 
int CRenderer::CV_r_pointlightshafts; 
int CRenderer::CV_r_distant_rain;

int CRenderer::CV_r_use_merged_posteffects;

int CRenderer::CV_r_PostProcessParamsBlending;
int CRenderer::CV_r_PostProcessReset;
int CRenderer::CV_r_PostProcessFilters;
int CRenderer::CV_r_PostProcessGameFx;
int CRenderer::CV_r_PostProcessScreenQuadTessX;
int CRenderer::CV_r_PostProcessScreenQuadTessY;
float CRenderer::CV_r_PostprocessParamsBlendingTimeScale;
int CRenderer::CV_r_PostProcessHUD3D;

int CRenderer::CV_r_showgammareference;

int CRenderer::CV_r_colorgrading;
int CRenderer::CV_r_colorgrading_selectivecolor;
int CRenderer::CV_r_colorgrading_levels;
int CRenderer::CV_r_colorgrading_filters;
int CRenderer::CV_r_colorgrading_dof;
int CRenderer::CV_r_colorgrading_charts;

int CRenderer::CV_r_fur;

int CRenderer::CV_r_cloudsupdatealways;
int CRenderer::CV_r_cloudsdebug;

int CRenderer::CV_r_showdyntextures;
int CRenderer::CV_r_ShowDynTexturesMaxCount;
ICVar *CRenderer::CV_r_ShowDynTexturesFilter;
int CRenderer::CV_r_shownormals;
int CRenderer::CV_r_showlines;
float CRenderer::CV_r_normalslength;
int CRenderer::CV_r_showtangents;
int CRenderer::CV_r_showtimegraph;
int CRenderer::CV_r_DebugFontRendering;
int CRenderer::CV_profileStreaming;
int CRenderer::CV_r_graphstyle;
int CRenderer::CV_r_showmt;

ICVar* CRenderer::CV_r_ShaderCompilerServer;
int CRenderer::CV_r_ShaderCompilerPort;

int CRenderer::CV_r_flares;

int CRenderer::CV_r_envcmwrite;
int CRenderer::CV_r_envcmresolution;  // 0-64,1-128,2-256
int CRenderer::CV_r_envtexresolution; // 0-64,1-128,2-256
float CRenderer::CV_r_waterupdateFactor;
float CRenderer::CV_r_waterupdateDistance;
float CRenderer::CV_r_envlcmupdateinterval;
float CRenderer::CV_r_envcmupdateinterval;
float CRenderer::CV_r_envtexupdateinterval;
int CRenderer::CV_r_waterreflections;
int CRenderer::CV_r_waterreflections_mgpu;
int CRenderer::CV_r_waterreflections_quality;
int CRenderer::CV_r_waterreflections_use_min_offset;
float CRenderer::CV_r_waterreflections_min_visible_pixels_update;
float CRenderer::CV_r_waterreflections_minvis_updatefactormul;
float CRenderer::CV_r_waterreflections_minvis_updatedistancemul;
int CRenderer::CV_r_watercaustics;
float CRenderer::CV_r_watercausticsdistance;
int CRenderer::CV_r_watercausticsdeferred;
int CRenderer::CV_r_water_godrays;
int CRenderer::CV_r_water_ripples;

int CRenderer::CV_r_texture_anisotropic_level;
int CRenderer::CV_r_texnoaniso;

int CRenderer::CV_r_reflections;	
int CRenderer::CV_r_reflections_quality;	
float CRenderer::CV_r_waterreflections_offset;	

int CRenderer::CV_r_reloadshaders;

int CRenderer::CV_r_cullbyclipplanes;
int CRenderer::CV_r_detailtextures;

int CRenderer::CV_r_detailnumlayers;
float CRenderer::CV_r_detailscale;
float CRenderer::CV_r_detaildistance;
int CRenderer::CV_r_texbindmode;
int CRenderer::CV_r_nodrawshaders;
int CRenderer::CV_r_nodrawnear;
float CRenderer::CV_r_DrawNearZRange;
float CRenderer::CV_r_DrawNearFarPlane;
float CRenderer::CV_r_drawnearfov;
int CRenderer::CV_r_profileshaders;
int CRenderer::CV_r_ProfileShadersSmooth;
int CRenderer::CV_r_ProfileShadersGroupByName;
ICVar *CRenderer::CV_r_excludeshader;
ICVar *CRenderer::CV_r_showonlyshader;
float CRenderer::CV_r_gamma;
float CRenderer::CV_r_contrast;
float CRenderer::CV_r_brightness;
int CRenderer::CV_r_nohwgamma;

int CRenderer::CV_r_scissor;

int CRenderer::CV_r_coronas;
float CRenderer::CV_r_coronafade;
float CRenderer::CV_r_coronasizescale;
float CRenderer::CV_r_coronacolorscale;

int CRenderer::CV_r_wireframe;
int CRenderer::CV_r_GetScreenShot;

int CRenderer::CV_r_character_nodeform;

int	CRenderer::CV_r_VegetationSpritesGenDebug;
int	CRenderer::CV_r_VegetationSpritesNoBend;
int CRenderer::CV_r_ZPassOnly;
int	CRenderer::CV_r_VegetationSpritesNoGen;
int	CRenderer::CV_r_VegetationSpritesTexRes;
int	CRenderer::CV_r_VegetationSpritesGenAlways;
int	CRenderer::CV_r_VegetationSpritesIgnoreAngle;
int	CRenderer::CV_r_VegetationAlphaTestOnly;

int CRenderer::CV_r_ShowVideoMemoryStats;

int CRenderer::CV_r_ShowRenderTarget;
int CRenderer::CV_r_ShowRenderTarget_FullScreen;

ICVar* CRenderer::CV_r_ShowTexture = NULL;

int CRenderer::CV_r_ShowLightBounds;

int CRenderer::CV_r_MergeRenderChunksForDepth;
int CRenderer::CV_r_TextureCompressor;

int CRenderer::CV_r_UseGSParticles;
int CRenderer::CV_r_Force3DcEmulation;

float CRenderer::CV_r_ZFightingDepthScale;
float CRenderer::CV_r_ZFightingExtrude;

int CRenderer::CV_r_useSRGB;
//float CRenderer::CV_r_TexelsPerMeter;

int CRenderer::CV_r_ConditionalRendering;
int	CRenderer::CV_r_PS3HalfResRendering;
int CRenderer::CV_r_PS3SaveRSXPerformanceTimings;
int CRenderer::CV_r_PS3VMemDefrag;
int CRenderer::CV_r_PS3VMemDefragDebug;
int CRenderer::CV_r_PS3ConstBufferAdjustment;
int CRenderer::CV_r_PS3SoftwareRasterizer;

int CRenderer::CV_r_Stereo;
float CRenderer::CV_r_StereoEyeDist;
float CRenderer::CV_r_StereoScreenDist;

float CRenderer::CV_r_FogDepthTest;
int CRenderer::CV_r_FogColorGradient;

float CRenderer::CV_r_FlashMatTexResQuality;
//////////////////////////////////////////////////////////////////////

#if !defined (XENON) && !defined(PS3) && !defined(NULL_RENDERER)

static void ShadersPrecache (IConsoleCmdArgs* Cmd)
{
  gRenDev->m_cEF.mfPrecacheShaders(false, false);
}
static void ShadersStats (IConsoleCmdArgs* Cmd)
{
  gRenDev->m_cEF.mfPrecacheShaders(false, true);
}
static void ShadersPrecacheLevels (IConsoleCmdArgs* Cmd)
{
  gRenDev->m_cEF.mfPrecacheShadersLevels();
}

static void ShadersPrecacheList (IConsoleCmdArgs* Cmd)
{
  gRenDev->m_cEF.mfPrecacheShaders(true, false);
}
static void ShadersStatsList (IConsoleCmdArgs* Cmd)
{
  gRenDev->m_cEF.mfPrecacheShaders(true, true);
}

static void ShadersOptimise (IConsoleCmdArgs* Cmd)
{
  if (CRenderer::CV_r_shadersuserfolder)
  {
    string str = string("%USER%/") + string(gRenDev->m_cEF.m_ShadersCache);
    iLog->Log("Optimize user folder: '%s'", gRenDev->m_cEF.m_ShadersCache);
    gRenDev->m_cEF.mfOptimiseShaders(str.c_str(), false);
  }
  uint32 nComp = CRenderer::CV_r_shadersintcompiler;
  if (CRenderer::CV_r_shadersxenon)
  {
    CRenderer::CV_r_shadersintcompiler = 0;

    CParserBin::SetupForXenon();
    CryLogAlways("\nStarting shaders optimizing for X360...");
    iLog->Log("Optimize game folder: '%s'", gRenDev->m_cEF.m_ShadersCache);
    gRenDev->m_cEF.mfOptimiseShaders(gRenDev->m_cEF.m_ShadersCache, false);
  }
  if (CRenderer::CV_r_shadersps3)
  {
    CRenderer::CV_r_shadersintcompiler = 0;

    CParserBin::SetupForPS3();
    CryLogAlways("\nStarting shaders optimizing for PS3...");
    iLog->Log("Optimize game folder: '%s'", gRenDev->m_cEF.m_ShadersCache);
    gRenDev->m_cEF.mfOptimiseShaders(gRenDev->m_cEF.m_ShadersCache, false);
  }
  if (CRenderer::CV_r_shadersdx9)
  {
    CRenderer::CV_r_shadersintcompiler = 1;

    CParserBin::SetupForD3D9();
    CryLogAlways("\nStarting shaders optimizing for DX9...");
    iLog->Log("Optimize game folder: '%s'", gRenDev->m_cEF.m_ShadersCache);
    gRenDev->m_cEF.mfOptimiseShaders(gRenDev->m_cEF.m_ShadersCache, false);
  }
  if (CRenderer::CV_r_shadersdx11)
  {
    CRenderer::CV_r_shadersintcompiler = 0;

    CParserBin::SetupForD3D10();
    CryLogAlways("\nStarting shaders optimizing for DX11...");
    iLog->Log("Optimize game folder: '%s'", gRenDev->m_cEF.m_ShadersCache);
    gRenDev->m_cEF.mfOptimiseShaders(gRenDev->m_cEF.m_ShadersCache, false);
  }
  CRenderer::CV_r_shadersintcompiler = nComp;



#if defined (DIRECT3D10)
  CParserBin::SetupForD3D10();


#elif defined (DIRECT3D9)
  CParserBin::SetupForD3D9();
#endif
}

static void ShadersMerge (IConsoleCmdArgs* Cmd)
{
  gRenDev->m_cEF.mfMergeShaders();
}
#endif

static void cmd_OverscanBorders( IConsoleCmdArgs *pParams )
{
	int argCount = pParams->GetArgCount();

	if (argCount > 1)
	{
		CRenderer::s_overscanBorders.x = clamp_tpl((float)atof(pParams->GetArg(1)), 0.0f, 25.0f) * 0.01f;

		if (argCount > 2)
		{
			CRenderer::s_overscanBorders.y = clamp_tpl((float)atof(pParams->GetArg(2)), 0.0f, 25.0f) * 0.01f;
		}
		else
		{
			CRenderer::s_overscanBorders.y = CRenderer::s_overscanBorders.x;
		}
	}
	else
	{
		gEnv->pLog->LogWithType(ILog::eInputResponse,"Overscan Borders: Left/Right %G %% , Top/Bottom %G %%",
			CRenderer::s_overscanBorders.x*100.0f, CRenderer::s_overscanBorders.y*100.0f);
	}
}


static void OnChange_CV_r_HDRRendering( ICVar* pCVar )
{
	ITimeOfDay* pTimeOfDay( gEnv->p3DEngine->GetTimeOfDay() );
	float time( pTimeOfDay->GetTime() );
	pTimeOfDay->SetTime( time, true );

	// FSAA requires HDR mode on
	// search for #LABEL_FSAA_HDR
	if(!pCVar->GetIVal())
	{
		// HDR was switched off
		ICVar *pFSAA = gEnv->pConsole->GetCVar("r_FSAA");

		if(pFSAA->GetIVal())
			pFSAA->Set(0);			// switch off FSAA
	}
}

static void OnChange_CV_r_TexelsPerMeter(ICVar* /*pCVar*/)
{
	ICVar* pCV_e_sketch_mode = gEnv->pConsole->GetCVar("e_sketch_mode");
	if (pCV_e_sketch_mode)
		pCV_e_sketch_mode->Set(gRenDev->CV_r_TexelsPerMeter > 0.0f ? 4 : 0);
}


static void GetLogVBuffersStatic(ICVar* pCVar )
{
	gRenDev->GetLogVBuffers();
}

bool CRenderer::DoCompressedNormalmapEmulation() const
{
	bool b8KTexturesSupported = gRenDev->GetMaxTextureSize()>=8192;
	bool bNVidia = (gRenDev->GetFeatures()&RFT_HW_NVIDIA)!=0; 
	bool bDriverSupportsCompressedNormalmaps = m_iDeviceSupportsComprNormalmaps!=0;

	bool bEnableEmulation = !bDriverSupportsCompressedNormalmaps;

	if(GetRenderType()==eRT_DX9)				// only on DX9 we have hardware that can switch
	switch(CV_r_Force3DcEmulation)
	{
		case 0:	
			break;

		case 1:	
			bEnableEmulation = true;
			break;

		case 2:
			if(bNVidia && !b8KTexturesSupported)			// pre G80 hardware (NVidia, texture size) emulates 3Dc support in driver
				bEnableEmulation=true;
			break;

		default: assert(0);
	}

	return bEnableEmulation;
}


// required to apply changes in game
static void ChangeForce3DcEmulation( ICVar *pVar )
{
	assert(pVar);

	if(gEnv->pRenderer->GetRenderType()!=eRT_DX9)				// only on DX9 we have hardware that can switch
		return;

	CryLog("r_Force3DcEmulation trigger reloading of all textures");
	gEnv->pRenderer->EF_ReloadTextures();
}


void CRenderer::ChangeGeomInstancingThreshold( ICVar *pVar )
{
	// get user value
	m_iGeomInstancingThreshold = CV_r_geominstancingthreshold;

	// auto
	if(m_iGeomInstancingThreshold<=0)
	{
		int nGPU = gRenDev->GetFeatures() & RFT_HW_MASK;

		if(nGPU==RFT_HW_ATI)
			CRenderer::m_iGeomInstancingThreshold = 2;				// seems to help in performance on all cards
		 else if(nGPU==RFT_HW_NVIDIA || nGPU==RFT_HW_NV4X)
			CRenderer::m_iGeomInstancingThreshold = 8;				//
		 else
			CRenderer::m_iGeomInstancingThreshold = 7;				// might be a good start - can be tweaked further
	}

	CryLogAlways(" used GeomInstancingThreshold is %d",m_iGeomInstancingThreshold);
}


CRenderer::CRenderer()
{ 
	if (!gRenDev)
    gRenDev = this;
  m_cEF.m_Bin.m_pCEF = &m_cEF;

  m_wireframe_mode = R_SOLID_MODE;
  m_wireframe_mode_prev = R_SOLID_MODE;

	REGISTER_CVAR2("r_DeferredShading", &CV_r_deferredshading, 1, VF_DUMPTODISK,
		"Toggles deferred shading\n"
		"Usage: r_DeferredShading [0/1/2/3]\n"
		"0 - off\n"
		"1 - on (Default)\n"
		"2 - enables interleaved/tiled rendering tests (Do not use)\n"
		"3 - Adds all lights to deferred pass");

#if defined( PS3 ) || defined( XENON )
  // all lights deferred - step 1 (consoles / no sun yet)
  CV_r_deferredshading = 3;
#endif

  REGISTER_CVAR2("r_DeferredShadingLightVolumes", &CV_r_deferredshadingLightVolumes, 1, VF_DUMPTODISK,
    "Toggles Light volumes for deferred shading.\n"
    "Usage: r_DeferredShadingLightVolumes [0/1]\n"
    "Default is 1 (enabled). 0 Disables. ");

	REGISTER_CVAR2("r_DeferredShadingCubeMaps", &CV_r_deferredshadingcubemaps, 1, VF_DUMPTODISK,
		"Toggles deferred cube maps.\n"
		"Usage: r_DeferredShadingCubeMaps [0/1]\n"
		"Default is 1 (enabled), 0 Disables");

	REGISTER_CVAR2("r_DeferredShadingHeightBasedAmbient", &CV_r_deferredshadingheightbasedambient, 0, VF_DUMPTODISK,
		"Toggles experimental height based ambient.\n"
		"Usage: r_DeferredShadingHeightBasedAmbient [0/1]\n"
		"Default is 1 (enabled), 0 Disables");

  REGISTER_CVAR2("r_DeferredShadingStencilPrepass", &CV_r_deferredshadingstencilprepass, 1, VF_DUMPTODISK,
    "Toggles deferred shading stencil pre pass.\n"
    "Usage: r_DeferredShadingStencilPrepass [0/1]\n"
    "Default is 1 (enabled), 0 Disables");

  REGISTER_CVAR2("r_DeferredShadingScissor", &CV_r_deferredshadingscissor, 1, VF_DUMPTODISK,
    "Toggles deferred shading scissor test.\n"
    "Usage: r_DeferredShadingScissor [0/1]\n"
    "Default is 1 (enabled), 0 Disables");

  int32 nDeferredShadingDepthBoundsDefault = 1;



  REGISTER_CVAR2("r_DeferredShadingDepthBoundsTest", &CV_r_deferredshadingdepthboundstest, nDeferredShadingDepthBoundsDefault, 
    VF_DUMPTODISK,
    "Toggles deferred shading depth bounds test.\n"
    "Usage: r_DeferredShadingDepthBoundsTest [0/1]\n"
    "Default is 1 (enabled). 0 Disables. ");

  REGISTER_CVAR2("r_DeferredShadingDebug", &CV_r_deferredshadingdebug, 0, VF_DUMPTODISK,
    "Toggles deferred shading debug.\n"
    "Usage: r_DeferredShadingDebug [0/1]\n"
    "  0 disabled (Default)\n"
		"  1 skip lights rendering (but still do cpu work)\n"
		"  2 only 1 light\n"
		"  3 Fillrate debug (brighter colors means more expensive)");

  REGISTER_CVAR2("r_DeferredShadingLightLodRatio", &CV_r_deferredshadinglightlodratio, 1.0f, VF_DUMPTODISK,
    "Sets deferred shading light intensity threshold for PS3.\n"
    "Usage: r_DeferredShadingLightLodRatio [value]\n"
    "Default is 0.1");
  







  REGISTER_CVAR2("r_DeferredShadingInterleavedAcc", &CV_r_deferredshadinginterleavedacc, 0, VF_DUMPTODISK,
    "Sets deferred lighting accumulation horizontal or vertical half res.\n"
    "Usage: r_DeferredShadingILTiles [value]\n"
    "Default is 4 tiles ");

	REGISTER_CVAR2("r_ambientPartialStencilRefresh", &CV_r_ambientPartialStencilRefresh, 1, VF_DUMPTODISK,
		"In enabled, uses AABB instead of FS quad to refresh Hi-S for VisAreas.\n"
		"Usage: r_ambientPartialStencilRefresh [0/1]\n");






	REGISTER_CVAR2("r_IrradianceVolumes", &CV_r_irradiancevolumes, 1, VF_CHEAT,
		"Toggles irradiance volumes.\n"
		"Usage: r_IrradianceVolumes [0/1]\n"
		"Default is 0 (off)");

#ifdef USE_HDR
  ICVar* pCV_r_HDRRendering = REGISTER_CVAR2("r_HDRRendering", &CV_r_HDRRendering, 2, VF_DUMPTODISK,
		"Toggles HDR rendering.\n"
		"Usage: r_HDRRendering [0/1]\n"
		"Default is 1 (on). Set to 0 to disable HDR rendering.");
	pCV_r_HDRRendering->SetOnChangeCallback( OnChange_CV_r_HDRRendering );
  REGISTER_CVAR2("r_HDRDebug", &CV_r_HDRDebug, 0, 0,
		"Toggles HDR debugging info (to debug HDR/eye adaptation)\n"
    "Usage: r_HDRDebug [0/1/2]\n"
		"0 off (default)\n"
		"1 to show some internal HDR textures on the screen\n"
		"2 to identify illegal colors (grey=normal, red=NotANumber, green=negative)");
#endif
  
	REGISTER_CVAR2("r_HDRLevel", &CV_r_HDRLevel, 3.0f, VF_DUMPTODISK,
		"HDR rendering range level (color multiplier tweak together with hdr offset)\n"
    "Usage: r_HDRLevel [Value]\n"
    "Default is 3.0f");

	REGISTER_CVAR2("r_HDROffset", &CV_r_HDROffset, 10.0f, VF_DUMPTODISK,
		"HDR rendering range offset (color multiplier tweak together with hdr level)\n"
		"Usage: r_HDROffset [Value]\n"
		"Default is 10.0f");

	REGISTER_CVAR2("r_HDRContrast", &CV_r_HDRContrast, 1.0f, VF_DUMPTODISK,
		"HDR Contrast\n"
		"Usage: r_HDRContrast [Value]\n"
		"Default is 1.0 (no change)");

	REGISTER_CVAR2("r_HDRContrastLuminanceBlend", &CV_r_HDRContrastLuminanceBlend, 1.0f, VF_DUMPTODISK,
		"HDR contrast luminance blend (allow changing contrast on darker areas only)\n"
		"Usage: r_HDRContrastLuminanceBlend [Value]\n"
		"Default is 1.0f (no change)");

	REGISTER_CVAR2("r_HDRVignetting", &CV_r_HDRVignetting, 1, VF_DUMPTODISK,
		"HDR viggneting\n"
		"Usage: r_HDRVignetting [Value]\n"
		"Default is 1 (enabled)");

	REGISTER_CVAR2("r_HDRRangeAdapt", &CV_r_HDRRangeAdapt, 0, VF_DUMPTODISK,
		"Enable/Disable HDR range adaptation (improve precision - minimize banding) \n"
		"Usage: r_HDRRangeAdapt [Value]\n"
		"Default is 1");

#if defined(PS3) || defined( XENON )
	// Enable by default on consoles
	CV_r_HDRRangeAdapt = 1;
#endif

	REGISTER_CVAR2("r_HDRRangeAdaptMax", &CV_r_HDRRangeAdaptMax, 1.0f, VF_DUMPTODISK,
		"Set HDR range adaptation max adaptation (improve precision - minimize banding) \n"
		"Usage: r_HDRRangeAdaptMax [Value]\n"
		"Default is 1.0f");	

	REGISTER_CVAR2("r_HDRRangeAdaptMaxRange", &CV_r_HDRRangeAdaptMaxRange, 4.0f, VF_DUMPTODISK,
		"Set HDR range adaptation max adaptation (improve precision - minimize banding) \n"
		"Usage: r_HDRRangeAdaptMaxRange [Value]\n"
		"Default is 4.0f");	

	REGISTER_CVAR2("r_HDRRangeAdaptLBufferMax", &CV_r_HDRRangeAdaptLBufferMax, 0.125f, VF_DUMPTODISK,
		"Set range adaptation max adaptation for light buffers (improve precision - minimize banding) \n"
		"Usage: r_HDRRangeAdaptLBufferMax [Value]\n"
		"Default is 0.25f");	

	REGISTER_CVAR2("r_HDRRangeAdaptLBufferMaxRange", &CV_r_HDRRangeAdaptLBufferMaxRange, 4.0f, VF_DUMPTODISK,
		"Set range adaptation max range adaptation for light buffers (improve precision - minimize banding) \n"
		"Usage: r_HDRRangeAdaptLBufferMaxRange [Value]\n"
		"Default is 4.0f");	


	REGISTER_CVAR2("r_HDRTexFormat", &CV_r_HDRTexFormat, 0, VF_DUMPTODISK,
		"HDR texture format. \n"
		"Usage: r_HDRTexFormat [Value] 0:(low precision - cheaper/faster), 1:(high precision)\n"
		"Default is 0");

	REGISTER_CVAR2("r_HDRForceUpdateTextures", &CV_r_HDRForceUpdateTextures, 0, VF_DUMPTODISK,
		"Forces updating HDR textures. \n");

	// Eye Adaption
	REGISTER_CVAR2("r_EyeAdaptationFactor", &CV_r_HDREyeAdaptationFactor, 0.5f, VF_DUMPTODISK,
		"HDR rendering eye adaptation factor (0 means no adaption to current scene luminance, 1 means full adaption)\n"
		"Usage: r_HDREyeAdaptionFactor [Value]\n"
		"Default is 0.5");
	REGISTER_CVAR2("r_EyeAdaptationBase", &CV_r_HDREyeAdaptationBase, EYEADAPTIONBASEDEFAULT, 0,
		"HDR rendering eye adaptation base value (smaller values result in brighter adaption)\n"
		"Usage: r_EyeAdaptationBase [Value]");
	REGISTER_CVAR2("r_EyeAdaptationSpeed", &CV_r_HDREyeAdaptationSpeed, 1.0f, 0,
		"HDR rendering eye adaptation speed\n"
		"Usage: r_EyeAdaptationSpeed [Value]");

	REGISTER_CVAR2("r_HDRRangeAdaptationSpeed", &CV_r_HDRRangeAdaptationSpeed, 4.0f, 0,
		"HDR range adaption speed\n"
		"Usage: r_HDRRangeAdaptationSpeed [Value]");

  REGISTER_CVAR2("r_HDRBrightThreshold", &CV_r_HDRBrightThreshold, 3.0f, VF_DUMPTODISK,
		"HDR rendering bright threshold.\n"
		"Usage: r_HDRBrightThreshold [Value]\n"
		"Default is 3.0");
  REGISTER_CVAR2("r_HDRBrightOffset", &CV_r_HDRBrightOffset, 6.0f, VF_DUMPTODISK,
		"HDR rendering bright offset.\n"
		"Usage: r_HDRBrightOffset [Value]\n"
		"Default is 6.0");
	REGISTER_CVAR2("r_HDRBrightLevel", &CV_r_HDRBrightLevel, 0.6f, VF_DUMPTODISK,
		"HDR rendering level (bloom multiplier, tweak together with threshold)\n"
		"Usage: r_HDRBrightLevel [Value]\n"
		"Default is 0.6");

	REGISTER_CVAR2("r_HDRBlueShift", &CV_r_HDRBlueShift, 1.0f, VF_DUMPTODISK,
		"HDR rendering blue shift.\n"
		"Usage: r_HDRBlueShift 0 to 1\n"
		"Default is 0 (disabled). Set to 1 to use max blue shift strength");

  REGISTER_CVAR2("r_Beams", &CV_r_beams, 3, 0,
    "Toggles light beams.\n"
    "Usage: r_Beams [0/1/2/3]\n"
    "Default is 3 (optimized beams with glow support). Set to 0 to disable beams or 2 to\n"
		"use fake beams. Set 1 for real beams, full resolution (slower). Set to 3 to use\n"
		"optimized and with glow support beams.");
#if defined(DIRECT3D10)
  REGISTER_CVAR2("r_BeamsSoftClip", &CV_r_beamssoftclip, 1, 0,
    "Toggles light beams clip type.\n"
    "Usage: r_BeamsSoftClip [0/1]\n"
    "Default is 1 (software clip beams). Set to 0 to enable hardware clipping.");
#else
  REGISTER_CVAR2("r_BeamsSoftClip", &CV_r_beamssoftclip, 1, 0,
    "Toggles light beams clip type.\n"
    "Usage: r_BeamsSoftClip [0/1]\n"
    "Default is 1 (software clip beams). Set to 0 to enable hardware clipping.");
#endif
  REGISTER_CVAR2("r_BeamsHelpers", &CV_r_beamshelpers, 0, 0,
    "Toggles light beams helpers drawing.\n"
    "Usage: r_BeamsHelpers [0/1]\n"
    "Default is 0 (disabled helpers). Set to 1 to enable drawing helpers.");
  REGISTER_CVAR2("r_BeamsMaxSlices", &CV_r_beamsmaxslices, 200, 0,
    "Number of volumetric slices allowed per light beam.\n"
    "Usage: r_BeamsMaxSlices [1-300]\n"
    "Default is 200 (high-spec).");
  REGISTER_CVAR2("r_BeamsDistFactor", &CV_r_beamsdistfactor, 0.01f, 0,
    "Distance between slices.\n"
    "Usage: r_BeamsDistFactor [fValue]\n"
    "Default is 0.01 (0.01 meters between slices).");
  ICVar *pCVar_GeomInst = REGISTER_CVAR2("r_GeomInstancingThreshold", &CV_r_geominstancingthreshold, 0, 0,
    "If the instance count gets bigger than the specified value the instancing feature is used.\n"
    "Usage: r_GeomInstancingThreshold [Num]\n"
    "Default is 0 (automatic depending on hardware, used value can be found in the log)");
	pCVar_GeomInst->SetOnChangeCallback(ChangeGeomInstancingThreshold);







#if defined (DIRECT3D9) || defined (OPENGL)
  REGISTER_CVAR2("r_GeomInstancing", &CV_r_geominstancing, 1, 0,
    "Toggles HW geometry instancing.\n"
    "Usage: r_GeomInstancing [0/1]\n"
    "Default is 1 (on). Set to 0 to disable geom. instancing.");
#elif defined (DIRECT3D10)
  REGISTER_CVAR2("r_GeomInstancing", &CV_r_geominstancing, 1, 0,
    "Toggles HW geometry instancing.\n"
    "Usage: r_GeomInstancing [0/1]\n"
    "Default is 1 (on). Set to 0 to disable geom. instancing.");
#endif

  REGISTER_CVAR2("r_ImpostersDraw", &CV_r_impostersdraw, 1, 0,
    "Toggles imposters drawing.\n"
    "Usage: r_ImpostersDraw [0/1]\n"
    "Default is 1 (on). Set to 0 to disable imposters.");
	REGISTER_CVAR2("r_ImposterRatio", &CV_r_imposterratio, 1, 0,
		"Allows to scale the texture resolution of imposters (clouds)\n"
		"Usage: r_ImposterRatio [1..]\n"
		"Default is 1 (1:1 normal). Bigger values can help to save texture space\n"
		"(e.g. value 2 results in 1/4 texture memory usage)");
  REGISTER_CVAR2("r_ImpostersUpdatePerFrame", &CV_r_impostersupdateperframe, 6000, 0,
    "How many kilobytes to update per-frame.\n"
    "Usage: r_ImpostersUpdatePerFrame [1000-30000]\n"
    "Default is 6000 (6 megabytes)");






















  REGISTER_CVAR2("r_CBStatic", &CV_r_CBStatic, 0, 0,
    "Toggles per-instance CBs as static.\n"
    "Usage: r_UseCBStatic [0/1]\n"
    "Default is 1 (on). Set to 0 to use dynamic update of CB's per-instance.");

  REGISTER_CVAR2("r_CBStaticDebug", &CV_r_CBStaticDebug, 0, 0,
    "Toggles debugging of per-instance CBs.\n"
    "Usage: r_UseCBStaticDebug [0/1]\n"
    "Default is 0 (off). Set to 1 to enable asserts when static CB content is differ from the actual instance content.");
	
	REGISTER_CVAR2("r_ZPrePass", &CV_r_ZPrePass, 0, 0,
		"Toggles Z pre pass optimizations.\n"
		"Usage: r_UseZPrePass [0/1]\n"
		"Default is 1 (on). Set to 0 to disable Z-pre-pass.");

	REGISTER_CVAR2("r_ZPrePassRadiusThreshold", &CV_r_ZPrePassRadiusThreshold, 5.0f, 0,
		"Toggles Z pre pass drawcall selection radius threshold.\n"
		"Usage: r_ZPrePassRadiusThreshold \n"
		"Default is 5 (meters)");

	REGISTER_CVAR2("r_ZPrePassDistanceThreshold", &CV_r_ZPrePassDistanceThreshold, 500.0f, 0,
		"Toggles Z pre pass drawcall selection distance threshold.\n"
		"Usage: r_ZPrePassDistanceThreshold \n"
		"Default is 500 (meters)");
	

  REGISTER_CVAR2("r_UseZPass", &CV_r_usezpass, 1, 0,
    "Toggles Z pass.\n"
    "Usage: r_UseZPass [0/1]\n"
    "Default is 1 (on). Set to 0 to disable Z-pass.");
	REGISTER_CVAR2("r_UseAlphaBlend", &CV_r_usealphablend, 1, 0,
		"Toggles alpha blended objects.\n"
		"Usage: r_UseAlphaBlend [0/1]\n"
		"Default is 1 (on). Set to 0 to disable all alpha blended object.");
	REGISTER_CVAR2("r_UseEdgeAA", &CV_r_useedgeaa, 1, 0,
		"Toggles edge blurring/antialiasing\n"
		"Usage: r_UseEdgeAA [0/1/2]\n"
		"Default is 1 (edge blurring)\n"
		"1 = activate edge blurring mode\n"
		"2 = activate edge antialiasing mode (previous version)");
  REGISTER_CVAR2("r_UseHWSkinning", &CV_r_usehwskinning, 1, 0,
    "Toggles HW skinning.\n"
    "Usage: r_UseHWSkinning [0/1]\n"
    "Default is 1 (on). Set to 0 to disable HW-skinning.");
  REGISTER_CVAR2("r_UseMaterialLayers", &CV_r_usemateriallayers, 2,0,
    "Enables material layers rendering.\n"
    "Usage: r_UseMaterialLayers [0/1/2]\n"
    "Default is 2 (optimized). Set to 1 for enabling but with optimization disabled (for debug).");

  REGISTER_CVAR2("r_UseSoftParticles", &CV_r_usesoftparticles, 1,0,
    "Enables soft particles.\n"
    "Usage: r_UseSoftParticles [0/1]");

  REGISTER_CVAR2("r_UseWaterParticles", &CV_r_usewaterparticles, 1,0,
    "Enables water particles.\n"
    "Usage: r_UseWaterParticles [0/1]");

  REGISTER_CVAR2("r_UseParticlesRefraction", &CV_r_useparticles_refraction, 1,0,
    "Enables refractive particles.\n"
    "Usage: r_UseParticlesRefraction [0/1]");

  REGISTER_CVAR2("r_UseParticlesGlow", &CV_r_useparticles_glow, 1,0,
    "Enables glow particles.\n"
    "Usage: CV_r_useparticles_glow [0/1]");

	REGISTER_CVAR2("r_UsePOM", &CV_r_usepom, 1, 0,
		"Enables Parallax Occlusion Mapping.\n"
		"Usage: r_UsePOM [0/1]");

  REGISTER_CVAR2("r_PostMSAA", &CV_r_PostMSAA, 0, 0,
    "Enables post process msaa.\n"
    "Usage: r_PostMSAA [0/1]");

	REGISTER_CVAR2("r_PostStereo", &CV_r_PostStereo, 0, 0,
		"Enables post process stereo view.\n"
		"Usage: r_PostStereo [0/1]");

  REGISTER_CVAR2("r_HairSortingQuality", &CV_r_hair_sorting_quality, 1, 0,
    "Enables higher quality hair sorting.\n"
    "Usage: r_HairSortingQuality [0/1]");

  REGISTER_CVAR2("r_MotionBlur", &CV_r_motionblur, 1,0,
    "Enables per object and screen motion blur.\n"
    "Usage: r_MotionBlur [0/1/2/3/4/101/102/103/104]\n"
    "Default is 1 (screen motion blur on).\n"
		"1 enables screen motion blur\n"
		"2 enables screen and object motion blur\n"
		"3 all motion blur and freezing\n"
		"4. only per object\n"
		"modes above 100 also enable motion blur in multiplayer");  
  REGISTER_CVAR2("r_MotionBlurShutterSpeed", &CV_r_motionblur_shutterspeed, 0.015f,0,
    "Sets motion blur camera shutter speed.\n"
    "Usage: r_MotionBlurShutterSpeed [0...1]\n"
    "Default is 0.015.");  

  REGISTER_CVAR2("r_MotionBlurFrameTimeScale", &CV_r_motionblurframetimescale, 1,0,
    "Enables motion blur.frame time scaling - visually nicer on lower frame rates\n"
    "Usage: r_MotionBlurFrameTimeScale [0/1]");

  REGISTER_CVAR2("r_MotionBlurDynamicQuality", &CV_r_motionblurdynamicquality, 1,0,
    "Enables motion blur.dynamic quality setting depending on movement amount\n"
    "Usage: r_MotionBlurDynamicQuality [0/1]");

  REGISTER_CVAR2("r_MotionBlurDynQualityRotationAccStiffness", &CV_r_motionblurdynamicquality_rotationacc_stiffness, 1.0f,0,
    "Usage: r_MotionBlurDynQualityRotationAccStiffness value (default 1.0f)\n");

  REGISTER_CVAR2("r_MotionBlurDynQualityRotationThreshold", &CV_r_motionblurdynamicquality_rotationthreshold, 1.0f,0,
    "Enables motion blur.dynamic quality setting depending on movement amount\n"
    "Usage: r_MotionBlurDynQualityRotationThreshold value (default 1.0f)\n");

  REGISTER_CVAR2("r_MotionBlurDynQualityTranslationThreshold", &CV_r_motionblurdynamicquality_translationthreshold, 1.0f,0,
    "Enables motion blur.dynamic quality setting depending on movement amount\n"
    "Usage: r_MotionBlurDynQualityTranslationThreshold value (default 1.0f)\n");

  REGISTER_CVAR2("r_DebugExtraSceneTargetFSAA", &CV_r_debug_extra_scenetarget_fsaa, 1,0,
    "Disables usage of shared sceneTarget RT in case its multisampled\n"
    "Usage: r_DebugSceneTargetNoFSAA [0/1]\n");

  REGISTER_CVAR2("r_CustomVisions", &CV_r_customvisions, 1,0,
    "Enables custom visions, like heatvision, binocular view, etc.\n"
    "Usage: r_CustomVisions [0/1/2]\n"
    "Default is 0 (disabled). 1 enables. 2 - cheaper version, no post processing");  

	REGISTER_CVAR2("r_DebugLayerEffect", &CV_r_DebugLayerEffect, 0,0,
		"Enables debug mode (independent from game code) for layer effects\n"
		"Usage: r_DebugLayerEffect [0/1/2/3/etc]\n"
		"Default is 0 (disabled). 1: 1st layer mode, etc");  

  REGISTER_CVAR2("r_Rain", &CV_r_rain, 1,0,
    "Enables rain rendering\n"
    "Usage: r_Rain [0/1/2]\n"
    "Default is 0 (disabled). 1 enables. 2 enables rain and rain fins");  

  REGISTER_CVAR2("r_RainAmount", &CV_r_rainamount, 1.0f,0,
    "Sets rain amount\n"
    "Usage: r_RainAmount");

  REGISTER_CVAR2("r_RainMaxViewDist", &CV_r_rain_maxviewdist, 32.0f,0,
    "Sets rain max view distance\n"
    "Usage: r_RainMaxViewDist");

  REGISTER_CVAR2("r_DepthOfField", &CV_r_dof, 3,0,
    "Enables depth of field.\n"
    "Usage: r_DepthOfField [0/1/2/3]\n"
    "Default is 0 (disabled). 1 enables, 2 hdr dof if supported. 3 alternative dof method");  

  REGISTER_CVAR2("r_DepthOfFieldBokeh", &CV_r_dof_bokeh, 0,0,
    "Sets depth of field bokeh type (only for dof mode 3).\n"
    "Usage: r_DepthOfFieldBokeh [0/1/etc]\n"
    "Default is 0 (isotropic/spherical).");  

	REGISTER_CVAR2("r_DepthOfFieldStencilPrepass", &CV_r_dof_stencil_prepass, 0,0,
		"Enables depth of field stencil prepass.\n"
		"Usage: r_DepthOfFieldStencilPrepass [0/1]\n"
		"Default is 0 (disabled). 1 enables");  

	REGISTER_CVAR2("r_DebugLightVolumes", &CV_r_DebugLightVolumes, 0, 0,
		"0=Disable\n"
		"1=Enable\n"
		"Usage: r_DebugLightVolumes[0/1]");

  REGISTER_CVAR2("r_UseShadowsPool", &CV_r_UseShadowsPool, 0, 0,
    "0=Disable\n"
    "1=Enable\n"
    "Usage: r_UseShadowsPool[0/1]");
  
  REGISTER_CVAR2("r_ShadowsSlopeScaleBias", &CV_r_ShadowsSlopeScaleBias, 1.8f, VF_DUMPTODISK, //3.5
    "Select shadow map blurriness if r_ShadowBlur is activated.\n"
    "Usage: r_ShadowBluriness [0.1 - 16]");
  REGISTER_CVAR2("r_ShadowsBias", &CV_r_ShadowsBias, 0.00008f, VF_DUMPTODISK, //-0.00002
    "Select shadow map blurriness if r_ShadowsBias is activated.\n"
    "Usage: r_ShadowsBias [0.1 - 16]");

  REGISTER_CVAR2("r_ShadowGenMode", &CV_r_ShadowGenMode, 1, 0,
                   "0=Use Frustums Mask\n"
                   "1=Regenerate all sides\n"
                   "Usage: r_ShadowGenMode [0/1]");

	REGISTER_CVAR2("capture_misc_render_buffers", &CV_capture_misc_render_buffers, 0, 0,
		"Captures internal render targets.\n"
		"Usage: capture_misc_render_buffers [0/1/2]\n"
		"0=Disable (default)\n"
		"1=Capture HDR, depth, shadow mask and AO buffers along with final framebuffer\n"
		"2=Capture stereo left and right buffers\n"
		"Note: Enable capturing via \"capture_frames 1\".");

  REGISTER_CVAR2("r_ShadowsUseClipVolume", &CV_r_ShadowsUseClipVolume, 0, VF_DUMPTODISK,
    ".\n"
    "Usage: r_ShadowsUseClipVolume [0=Disable/1=Enable");

  REGISTER_CVAR2("r_ShadowBlur", &CV_r_shadowblur, 3, VF_DUMPTODISK,
    "Selected shadow map screenspace blurring technique.\n"
    "Usage: r_ShadowBlur [0=no blurring(fastest)/1=blur/2=blur/3=blur without leaking(slower)]");
  REGISTER_CVAR2("r_ShadowTexFormat", &CV_r_shadowtexformat, 4, 0,
    "0=use R16G16 texture format for depth map, 1=try to use R16 format if supported as render target\n"
    "2=use R32F texture format for depth map\n"
    "3=use ATI's DF24 texture format for depth map\n"
    "4=use NVIDIA's D24S8 texture format for depth map\n"
    "5=use NVIDIA's D16 texture format for depth map\n"
    "Usage: r_ShadowTexFormat [0-5]");
  
  REGISTER_CVAR2("r_ShadowsDeferredMode", &CV_r_ShadowsDeferredMode, 1, 0,
    "0=Quad light bounds\n"
    "1=Use light volumes\n"
    "Usage: r_ShadowsDeferredMode [0/1]");
  REGISTER_CVAR2("r_ShadowsMaskResolution", &CV_r_ShadowsMaskResolution, 0, 0,
    "0=per pixel shadow mask\n"
    "1=horizontal half resolution shadow mask\n"
    "2=horizontal and vertical half resolution shadow mask\n"
    "Usage: r_ShadowsMaskResolution [0/1/2]");
  REGISTER_CVAR2("r_ShadowsMaskDownScale", &CV_r_ShadowsMaskDownScale, 0, 0,
    "Saves video memory by using lower resolution for shadow masks except first one\n"
    "0=per pixel shadow mask\n"
    "1=half resolution shadow mask\n"
    "Usage: r_ShadowsMaskDownScale [0/1]");
  REGISTER_CVAR2("r_ShadowsStencilPrePass", &CV_r_ShadowsStencilPrePass, 1, 0,
    "1=Use Stencil pre-pass for shadows\n"
    "Usage: r_ShadowsStencilPrePass [0/1]");
  REGISTER_CVAR2("r_ShadowsDepthBoundNV", &CV_r_ShadowsDepthBoundNV, 0, 0,
    "1=use NV Depth Bound extension\n"
    "Usage: CV_r_ShadowsDepthBoundNV [0/1]");
  REGISTER_CVAR2("r_ShadowsForwardPass", &CV_r_ShadowsForwardPass, 1, 0,
    "1=use Forward prepare depth maps pass\n"
    "Usage: CV_r_ShadowsForwardPass [0/1]");
  REGISTER_CVAR2("r_ShadowBluriness", &CV_r_shadowbluriness, 1.0f, VF_DUMPTODISK,
    "Select shadow map blurriness if r_ShadowBlur is activated.\n"
    "Usage: r_ShadowBluriness [0.1 - 16]");
	REGISTER_CVAR2("r_ShadowJittering", &CV_r_shadow_jittering, 3.4f, 0,
		"Activate shadow map jittering.\n"
		"Usage: r_ShadowJittering [0=off, 1=on]");
	REGISTER_CVAR2("r_VarianceShadowMapBlurAmount", &CV_r_VarianceShadowMapBlurAmount, 1.0f, VF_DUMPTODISK,
		"Activate shadow map blur.\n"
		"Usage: r_VarianceShadowMapBlurAmount [0=deactivate, >0 to specify blur amount (1=normal)]\n");
  REGISTER_CVAR2("r_DebugLights", &CV_r_debuglights, 0,VF_CHEAT,
		"Display dynamic lights for debugging.\n"
		"Usage: r_DebugLights [0/1/2/3]\n"
		"Default is 0 (off). Set to 1 to display centers of light sources,\n"
		"or set to 2 to display light centers and attenuation spheres, 3 to get light properties to the screen");
  REGISTER_CVAR2( "r_ShadowsGridAligned", &CV_r_ShadowsGridAligned, 1, VF_DUMPTODISK,
      "Selects algorithm to use for shadow mask generation:\n"
      "0 - Disable shadows snapping\n"
      "1 - Enable shadows snapping" );
  REGISTER_CVAR2( "r_ShadowGenGS", &CV_r_ShadowGenGS, 0, VF_DUMPTODISK,
    "Use geometry shader for shadow map generation (DX11 only, don't change at runtime)\n"
		"Usage: r_ShadowGenGS [0=off, 1=on]\n");
  REGISTER_CVAR2( "r_ShadowPass", &CV_r_ShadowPass, 1, 0,
    "Process shadow pass" );
  REGISTER_CVAR2( "r_ShadowGen", &CV_r_ShadowGen, 1, 0,
    "0=disable shadow map updates, 1=enable shadow map updates" );

	REGISTER_CVAR2( "r_RenderMeshHashGridUnitSize", &CV_r_RenderMeshHashGridUnitSize, .5f, 0, "Controls density of render mesh triangle indexing structures" );

	REGISTER_CVAR2( "r_SSAO", &CV_r_SSAO, 1, 0, 
		"Screen space ambient occlusion:\n"
		"0 - disabled\n"
		"1 - fast method\n"
		"2 - quality method\n"
		"3 - old method (fast but more noise)"
	);

  REGISTER_CVAR2( "r_SSAO_normals", &CV_r_SSAO_normals, 1, 0, "Enable usage of scene normals if deferred shading is activated" );
	REGISTER_CVAR2( "r_SSAODownscale", &CV_r_SSAO_downscale, 0, 0, "Use downscaled computations for SSAO" );
	REGISTER_CVAR2( "r_SSAO_downscale_ztarget", &CV_r_SSAO_downscale_ztarget, 1, 0, "Use downscaled version of z-target" );
  REGISTER_CVAR2( "r_SSAO_downscale_result_mask", &CV_r_SSAO_downscale_result_mask, 0, 0, "Downscale final mask" );
	REGISTER_CVAR2( "r_SSAO_amount", &CV_r_SSAO_amount,	   1.00f, 0, "Controls how much SSAO affects ambient" );
  REGISTER_CVAR2( "r_SSAO_darkening", &CV_r_SSAO_darkening,	   0.075f, 0, "Controls how much SSAO darkens flat open surfaces" );
	REGISTER_CVAR2( "r_SSAO_radius",	&CV_r_SSAO_radius,	 2.00f, 0, "Controls size of area tested" );
	REGISTER_CVAR2( "r_SSAO_quality", &CV_r_SSAO_quality,	   2, 0, "SSAO shader quality" );

	REGISTER_CVAR2( "r_SSGI",        &CV_r_SSGI,					0,		0, "SSGI toggle" );
	REGISTER_CVAR2( "r_SSGI_amount", &CV_r_SSGI_amount,	  1.0f, 0, "SSGI effect multiplier" );
	REGISTER_CVAR2( "r_SSGI_radius", &CV_r_SSGI_radius,	  0.1f, 0, "SSGI kernel radius" );
	REGISTER_CVAR2( "r_SSGI_quality",&CV_r_SSGI_quality,  2,    0, "SSGI quality level" );
	REGISTER_CVAR2( "r_SSGI_blur",   &CV_r_SSGI_blur,	    1,    0, "SSGI enable blur" );

  REGISTER_CVAR2( "r_TerrainAO", &CV_r_TerrainAO, 7, 0, "7=Activate terrain AO deferred passes" );
  REGISTER_CVAR2( "r_TerrainAO_FadeDist", &CV_r_TerrainAO_FadeDist, 8, 0, "Controls sky light fading in tree canopy in Z direction" );

  REGISTER_CVAR2("r_LightsSinglePass", &CV_r_lightssinglepass, 1, VF_NULL,"");
  
  REGISTER_CVAR2("r_ShowDynTextures",&CV_r_showdyntextures,0, NULL,
	  "Display a dyn. textures, filtered by r_ShowDynTexturesFilter\n"
	  "Usage: r_ShowDynTextures 0/1/2\n"
	  "Default is 0. Set to 1 to show all dynamic textures or 2 to display only the ones used in this frame\n"
    "Textures are sorted by memory usage");

  REGISTER_CVAR2("r_ShowDynTexturesMaxCount",&CV_r_ShowDynTexturesMaxCount,36, NULL,
    "Allows to adjust number of textures shown on the screen\n"
    "Usage: r_ShowDynTexturesMaxCount [1...36]\n"
    "Default is 36");

  CV_r_ShowDynTexturesFilter = REGISTER_STRING("r_ShowDynTexturesFilter","*", NULL,
    "Usage: r_ShowDynTexturesFilter *end\n"
		"Usage: r_ShowDynTexturesFilter *mid*\n"
		"Usage: r_ShowDynTexturesFilter start*\n"
    "Default is *. Set to 'pattern' to show only specific textures (activate r_ShowDynTextures)");

	CV_r_ShaderCompilerServer = REGISTER_STRING("r_ShaderCompilerServer","sc_server", NULL,
		"Usage: r_ShaderCompilerIP localhost \n"
		"Default is 8core5 ");

	REGISTER_CVAR2("r_ShaderCompilerPort", &CV_r_ShaderCompilerPort, 61453, NULL,
		"set user defined port of the shader compile server.\n"
		"Usage: r_ShaderCompilerPort 61453 #\n"
		"Default is 61453");

	REGISTER_CVAR2("r_RC_AutoInvoke", &CV_r_rc_autoinvoke, (gEnv->pSystem->IsDevMode() ? 1:0) , 0,
		"Enable calling the resource compiler (rc.exe) to compile TIF file to DDS files if the date check\n"
		"shows that the destination is older or does not exist.\n"
		"Usage: r_RC_AutoInvoke 0 (default is 1)");

  REGISTER_CVAR2("r_Glow", &CV_r_glow,1, 0,
    "Toggles the glow effect.\n"
    "Usage: r_Glow [0/1]\n"
    "Default is 0 (off). Set to 1 to enable glow effect.");

  REGISTER_CVAR2("r_GlowAnamorphicFlares", &CV_r_glowanamorphicflares, 0, 0,
    "Toggles the anamorphic flares effect.\n"
    "Usage: r_GlowAnamorphicFlares [0/1]\n"
    "Default is 0 (off). Set to 1 to enable.");

  REGISTER_CVAR2("r_NightVision", &CV_r_nightvision, 1, 0,
    "Toggles nightvision enabling.\n"
    "Usage: r_NightVision [0/1]\n"
    "Default is 1 (on). Set to 0 to completely disable nightvision.");

  REGISTER_CVAR2("r_refraction", &CV_r_refraction, 1, 0,
		"Enables refraction.\n"
		"Usage: r_refraction [0/1]\n"
		"Default is 1 (on). Set to 0 to disable.");  

  REGISTER_CVAR2("r_sunshafts", &CV_r_sunshafts, 1, 0,
    "Enables sun shafts.\n"
    "Usage: r_sunshafts [0/1]\n"
    "Default is 1 (on). Set to 0 to disable.");  

  REGISTER_CVAR2("r_pointslightshafts", &CV_r_pointlightshafts, 0, 0,
    "Enables point light shafts.\n"
    "Usage: r_pointslightshafts [0/1]\n"
    "Default is 1 (on). Set to 0 to disable.");  

	REGISTER_CVAR2("r_useMergedPostEffects", &CV_r_use_merged_posteffects, 1, 0,
		"Enables merged version of post effects whenever possible.\n"
		"Usage: r_use_merged_posteffects [0/1]\n"
		"Default is 1 (on). Set to 0 to disable.");

	
  REGISTER_CVAR2("r_Distant_rain", &CV_r_distant_rain, 1, 0,
    "Enables distant rain rendering.\n"
    "Usage: r_distant_rain [0/1]\n"
    "Default is 1 (on). Set to 0 to disable.");  
  
	REGISTER_CVAR2("r_PostProcessEffects", &CV_r_PostProcess, 1, VF_CHEAT,
		"Enables post processing special effects.\n"
		"Usage: r_PostProcessEffects [0/1/2]\n"
		"Default is 1 (enabled). 2 enables and displays active effects"); 

  REGISTER_CVAR2("r_PostProcessParamsBlending", &CV_r_PostProcessParamsBlending, 1, 0,
    "Enables post processing effects parameters smooth blending\n"
    "Usage: r_PostProcessEffectsParamsBlending [0/1]\n"
    "Default is 1 (enabled)."); 

	REGISTER_CVAR2("r_PostprocessParamsBlendingTimeScale", &CV_r_PostprocessParamsBlendingTimeScale, 7.0f, 0,
		"Sets post processing effects parameters smooth blending time scale\n"
		"Usage: r_PostprocessParamsBlendingTimeScale [scale]\n"
		"Default is 4.0f."); 

  REGISTER_CVAR2("r_PostProcessFilters", &CV_r_PostProcessFilters, 1, VF_CHEAT,
    "Enables post processing special effects filters.\n"
    "Usage: r_PostProcessEffectsFilters [0/1]\n"
    "Default is 1 (enabled). 0 disabled"); 

  REGISTER_CVAR2("r_PostProcessGameFx", &CV_r_PostProcessGameFx, 1, VF_CHEAT,
    "Enables post processing special effects game fx.\n"
    "Usage: r_PostProcessEffectsGameFx [0/1]\n"
    "Default is 1 (enabled). 0 disabled"); 

  REGISTER_CVAR2("r_PostProcessReset", &CV_r_PostProcessReset, 0, VF_CHEAT,
    "Enables post processing special effects reset.\n"
    "Usage: r_PostProcessEffectsReset [0/1]\n"
    "Default is 0 (disabled). 1 enabled"); 

	REGISTER_CVAR2("r_PostProcessScreenQuadTessX", &CV_r_PostProcessScreenQuadTessX, 0, 0,
		"Toggles higher tessellation for fullscreen quad.\n");	
	REGISTER_CVAR2("r_PostProcessScreenQuadTessY", &CV_r_PostProcessScreenQuadTessY, 0, 0,
		"Toggles higher tessellation for fullscreen quad.\n");	

	REGISTER_CVAR2("r_PostProcessHUD3D", &CV_r_PostProcessHUD3D, 0, 0,
		"Toggles 3d hud post processing.\n");	

  REGISTER_CVAR2("r_ShowGammaReference", &CV_r_showgammareference, 0, 0,
    "Enables display of gamma reference - useful for monitor/tv calibration.\n"
    "Usage: r_ShowGammaReference [0/1]"); 

  REGISTER_CVAR2("r_ColorGrading", &CV_r_colorgrading, 1, 0,
    "Enables color grading.\n"
    "Usage: r_ColorGrading [0/1]");

  REGISTER_CVAR2("r_Fur", &CV_r_fur, 0, 0,
    "Enables fur rendering.\n"
    "Usage: r_Fur [0/1]\n");

  REGISTER_CVAR2("r_ColorGradingSelectiveColor", &CV_r_colorgrading_selectivecolor, 1, 0,
    "Enables color grading.\n"
    "Usage: r_ColorGradingSelectiveColor [0/1]");

  REGISTER_CVAR2("r_ColorGradingLevels", &CV_r_colorgrading_levels, 1, 0,
    "Enables color grading.\n"
    "Usage: r_ColorGradingLevels [0/1]");

  REGISTER_CVAR2("r_ColorGradingFilters", &CV_r_colorgrading_filters, 1, 0,
    "Enables color grading.\n"
    "Usage: r_ColorGradingFilters [0/1]");

  REGISTER_CVAR2("r_ColorGradingDof", &CV_r_colorgrading_dof, 1, 0,
    "Enables color grading dof control.\n"
    "Usage: r_ColorGradingDof [0/1]");

//	REGISTER_CVAR2("r_ColorGradingCharts", &CV_r_colorgrading_charts, 1, 0, 
//		"Enables color grading via color charts.\n"
//		"Usage: r_ColorGradingCharts [0/1]");

	REGISTER_CVAR2("r_CloudsUpdateAlways", &CV_r_cloudsupdatealways, 0, 0,
		"Toggles updating of clouds each frame.\n"
		"Usage: r_CloudsUpdateAlways [0/1]\n"
		"Default is 0 (off)");
  REGISTER_CVAR2("r_CloudsDebug", &CV_r_cloudsdebug, 0, 0,
    "Toggles debugging mode for clouds."
    "Usage: r_CloudsDebug [0/1/2]\n"
    "Usage: r_CloudsDebug = 1: render just screen imposters\n"
    "Usage: r_CloudsDebug = 2: render just non-screen imposters\n"
    "Default is 0 (off)");

	REGISTER_CVAR2("r_DynTexMaxSize", &CV_r_dyntexmaxsize, 48, VF_NULL,""); // 48 Mb
  REGISTER_CVAR2("r_TexPreallocateAtlases", &CV_r_texpreallocateatlases, 1, VF_NULL,"");

#if !defined(XENON) && !defined(PS3)
  REGISTER_CVAR2("r_TexAtlasSize", &CV_r_texatlassize, 1024, VF_NULL,""); // 1024x1024
  REGISTER_CVAR2("r_TexPostponeLoading", &CV_r_texpostponeloading, 1,VF_NULL,"");
  REGISTER_CVAR2("r_DynTexAtlasCloudsMaxSize", &CV_r_dyntexatlascloudsmaxsize, 32,VF_NULL,""); // 32 Mb
  REGISTER_CVAR2("r_DynTexAtlasSpritesMaxSize", &CV_r_dyntexatlasspritesmaxsize, 32,VF_NULL,""); // 32 Mb
  REGISTER_CVAR2("r_DynTexAtlasVoxTerrainMaxSize", &CV_r_dyntexatlasvoxterrainsize, 250,VF_NULL,""); // this pool is never preallocated
  REGISTER_CVAR2("r_DynTexAtlasDynTexSrcSize", &CV_r_dyntexatlasdyntexsrcsize, 16,VF_NULL,""); // this pool is never preallocated







#endif
  REGISTER_CVAR2("r_TexMaxAnisotropy", &CV_r_texmaxanisotropy, 8, VF_REQUIRE_LEVEL_RELOAD,"");
  REGISTER_CVAR2("r_TexMaxSize", &CV_r_texmaxsize, 0,VF_CHEAT,"");
  REGISTER_CVAR2("r_TexMinSize", &CV_r_texminsize, 64,VF_CHEAT,"");

  REGISTER_CVAR2("r_TexBumpResolution", &CV_r_texbumpresolution, 0, VF_DUMPTODISK,
		"Reduces texture resolution.\n"
		"Usage: r_TexBumpResolution [0/1/2 etc]\n"
		"When 0 (default) texture resolution is unaffected, 1 halves, 2 quarters etc.");

  REGISTER_CVAR2("r_TexResolution", &CV_r_texresolution, 0, VF_DUMPTODISK,
		"Reduces texture resolution.\n"
		"Usage: r_TexResolution [0/1/2 etc]\n"
		"When 0 (default) texture resolution is unaffected, 1 halves, 2 quarters etc.");

  REGISTER_CVAR2("r_TexLMResolution", &CV_r_texlmresolution, 0, VF_DUMPTODISK,
		"Reduces texture resolution.\n"
		"Usage: r_TexLMResolution [0/1/2 etc]\n"
		"When 0 (default) texture resolution is unaffected, 1 halves, 2 quarters etc.");

  REGISTER_CVAR2( "r_TexSkyResolution", &CV_r_texskyresolution, 0, VF_DUMPTODISK,"" );

  REGISTER_CVAR2("r_TexNoAniso", &CV_r_texnoaniso, 0, VF_DUMPTODISK,"");
  REGISTER_CVAR2("r_Texture_Anisotropic_Level", &CV_r_texture_anisotropic_level, 1, VF_DUMPTODISK,"");
  REGISTER_CVAR2("r_TexNormalMapType", &CV_r_texnormalmaptype, 1, VF_REQUIRE_LEVEL_RELOAD,"");
  REGISTER_CVAR2("r_TexHWMipsGeneration", &CV_r_texhwmipsgeneration, 1,VF_NULL,"");
  REGISTER_CVAR2("r_TexGrid", &CV_r_texgrid, 0,VF_NULL,"");
  REGISTER_CVAR2("r_TexLog", &CV_r_texlog, 0, 0,
    "Configures texture information logging.\n"
    "Usage:	r_TexLog #\n"
    "where # represents:\n"
    "	0: Texture logging off\n"
    "	1: Texture information logged to screen\n"
    "	2: All loaded textures logged to 'UsedTextures.txt'\n"
    "	3: Missing textures logged to 'MissingTextures.txt");
  REGISTER_CVAR2("r_TexNoLoad", &CV_r_texnoload, 0, 0,
    "Disables loading of textures.\n"
    "Usage:	r_TexNoLoad [0/1]\n"
    "When 1 texture loading is disabled.");

  REGISTER_CVAR2("r_TexturesStreamPoolSize", &CV_r_texturesstreampoolsize, 128, VF_NULL,
		"Size of pool for textures streaming in MB.\n"
		"If r_TexturesStreaming is set to 2, this parameter is chosen automatically for PC.\n"
		"Default is 128(MB) for PC, 80(MB) for PS3 & XBox 360.");
  REGISTER_CVAR2("r_TexturesStreamingSync", &CV_r_texturesstreamingsync, 0,VF_NULL,
		"Force only synchronous texture streaming.\n"
		"All textures will be streamed in the main thread. Useful for debug purposes.\n"
		"Usage: r_TexturesStreamingSync [0/1]\n"
		"Default is 0 (off).");
  REGISTER_CVAR2("r_TexturesStreamingMaxAsync", &CV_r_texturesstreamingmaxasync, 0.25f,VF_NULL,
		"Maximum single async streaming request size, in MB[default = 0.25MB]. Applicable for PC only.\n"
		"Usage: r_TexturesStreamingMaxAsync [size]\n"
		"Default is 0.25(MB) for PC.");
  REGISTER_CVAR2("r_TexturesStreamingMipBias", &CV_r_TexturesStreamingMipBias, 0,VF_NULL,
    "Controls how texture LOD depends from distance to the objects.\n"
    "Increasing this value will reduce amount of memory required for textures.\n"
    "Usage: r_TexturesStreamingMipBias [0...4]\n"
    "Default is 0.");
  REGISTER_CVAR2("r_TexturesStreamingNoUpload", &CV_r_texturesstreamingnoupload, 0,VF_NULL,
		"Disable uploading data into texture from system memory. Useful for debug purposes.\n"
		"Usage: r_TexturesStreamingNoUpload [0/1]\n"
		"Default is 0 (off).");
	REGISTER_CVAR2("r_TexturesStreamingOnlyVideo", &CV_r_texturesstreamingonlyvideo, 0,VF_NULL,
		"Don't store system memory copy of texture. Applicable only for PC and PS3.\n"
		"On Xenon it's on[1] by default and cannot be changed.\n"
		"Usage: r_TexturesStreamingOnlyVideo [0/1]\n"
		"Default is 0 (off) for PC and PS3, 1(on always) for XBox 360.");
  REGISTER_CVAR2("r_TexturesStreaming", &CV_r_texturesstreaming, 0, VF_REQUIRE_APP_RESTART,
		"Enables direct streaming of textures from disk during game.\n"
		"Usage: r_TexturesStreaming [0/1/2]\n"
		"Default is 0 (off). All textures save in native format with mips in a\n"
		"cache file. Textures are then loaded into texture memory from the cache.");
  REGISTER_CVAR2("r_TexturesStreamingDebug", &CV_r_texturesstreamingdebug, 0, VF_CHEAT,
		"Enables textures streaming debug mode. (Log uploads and remove unnecessary mip levels)\n"
		"Usage: r_TexturesStreamingDebug [0/1/2]\n"
		"Default is 0 (off)."
		"1 - texture streaming log."
		"2 - Show textures hit-parade based on streaming priorities"
		"3 - Show textures hit-parade based on the memory consumed");
  REGISTER_CVAR2("r_TexturesFilteringQuality", &CV_r_texturesfilteringquality, 0, VF_REQUIRE_LEVEL_RELOAD,
    "Configures texture filtering adjusting.\n"
    "Usage: r_TexturesFilteringQuality [#]\n"
    "where # represents:\n"
    "	0: Highest quality\n"
    "	1: Medium quality\n"
    "	2: Low quality");
	REGISTER_CVAR2("r_TextureLodDistanceRatio", &CV_r_TextureLodDistanceRatio, -1, 0,
    "Controls dynamic LOD system for textures used in materials.\n"
    "Usage: r_TextureLodDistanceRatio [-1, 0 and bigger]\n"
    "Default is -1 (completely off). Value 0 will set full LOD to all textures used in frame.\n"
    "Values bigger than 0 will activate texture LOD selection depending on distance to the objects.");

	REGISTER_CVAR2("r_TextureLodMaxLod", &CV_r_TextureLodMaxLod, 1, 0,
		"Controls dynamic LOD system for textures used in materials.\n"
		"Usage: r_TextureLodMaxLod [1 or bigger]\n"
		"Default is 1 (playing between lod 0 and 1). Value 0 will set full LOD to all textures used in frame");
#ifndef STRIP_RENDER_THREAD
  REGISTER_CVAR2("r_MultiThreaded", &CV_r_multithreaded, 0, 0,
    "0=disabled, 1=enabling rendering in separate thread,\n"
    "2(default)=automatic detection\n"
    "should be activated before rendering");
#endif
  REGISTER_CVAR2("r_MultiGPU", &CV_r_multigpu, 2, 0,
		"0=disabled, 1=extra overhead to allow SLI(NVidia) or Crossfire(ATI),\n"
		"2(default)=automatic detection (currently SLI only, means off for ATI)\n"
		"should be activated before rendering");
  REGISTER_CVAR2("r_ValidateDraw", &CV_r_validateDraw, 0, 0,
    "0=disabled, 1=validate each DIP (meshes consistency, shaders, declarations, etc)");
  REGISTER_CVAR2("r_ProfileGPU", &CV_r_profileGPU, 0, VF_CHEAT,
    "0=disabled, 1=profile each DIP performance (may cause very low frame rate)\n"
		"r_ProfileShaders or r_Stats needs to be activated to see the statistics");

  REGISTER_CVAR2("r_FSAA", &CV_r_fsaa, 0, VF_DUMPTODISK | VF_REQUIRE_APP_RESTART,
		"Enables multisampled antialiasing.\n"
		"Usage: r_FSAA [0/1]\n"
		"Default: 0 (off).");
  REGISTER_CVAR2("r_FSAA_samples", &CV_r_fsaa_samples, 4, VF_DUMPTODISK | VF_REQUIRE_APP_RESTART,
		"Number of subsamples used when multisampled antialiasing is enabled.\n"
		"Usage: r_FSAA_samples N (where N is a number >= 0). Attention, N must be supported by given video hardware!\n"
		"Default: 4. Please note that various hardware implements special FSAA modes via certain combinations of\n"
		"r_FSAA_quality and r_FSAA_samples. See /game/config/FSAAProfiles*.txt for samples.");
  REGISTER_CVAR2("r_FSAA_quality", &CV_r_fsaa_quality, 0, VF_DUMPTODISK | VF_REQUIRE_APP_RESTART,
		"Quality level used when multisampled antialiasing is enabled.\n"
		"Usage: r_FSAA_quality N (where N is a number >= 0). Attention, N must be supported by given video hardware!\n"
		"Default: 0. Please note that various hardware implements special FSAA modes via certain combinations of\n"
		"r_FSAA_quality and r_FSAA_samples. See /game/config/FSAAProfiles*.txt for samples.");

  REGISTER_CVAR2("r_ATOC", &CV_r_atoc, 0, VF_DUMPTODISK,"");

  REGISTER_CVAR2("r_ShowNormals", &CV_r_shownormals, 0, VF_CHEAT,
		"Toggles visibility of normal vectors.\n"
		"Usage: r_ShowNormals [0/1]"
		"Default is 0 (off).");
  REGISTER_CVAR2("r_ShowLines", &CV_r_showlines, 0, VF_CHEAT,
		"Toggles visibility of wireframe overlay.\n"
		"Usage: r_ShowLines [0/1]\n"
		"Default is 0 (off).");
  REGISTER_CVAR2("r_NormalsLength", &CV_r_normalslength, 0.2f, VF_CHEAT,
		"Sets the length of displayed vectors.\n"
		"r_NormalsLength 0.2\n"
		"Default is 0.2 (meters). Used with r_ShowTangents and r_ShowNormals.");
  REGISTER_CVAR2("r_ShowTangents", &CV_r_showtangents, 0, VF_CHEAT,
		"Toggles visibility of three tangent space vectors.\n"
		"Usage: r_ShowTangents [0/1]\n"
		"Default is 0 (off).");
  REGISTER_CVAR2("r_ShowTimeGraph", &CV_r_showtimegraph, 0, 0,
		"Configures graphic display of frame-times.\n"
		"Usage: r_ShowTimeGraph [0/1/2]\n"
		"	1: Graph displayed as points."
		"	2: Graph displayed as lines."
		"Default is 0 (off).");
#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
	REGISTER_CVAR2("r_DebugFontRendering", &CV_r_DebugFontRendering, 0, VF_CHEAT, 
		"0=off, 1=display various features of the font rendering to verify function and to document usage");
#endif // EXCLUDE_DOCUMENTATION_PURPOSE
  REGISTER_CVAR2("profileStreaming", &CV_profileStreaming, 0, 0,
    "Profiles streaming of different assets.\n"
    "Usage: profileStreaming [0/1/2]\n"
    "	1: Graph displayed as points."
    "	2: Graph displayed as lines."
    "Default is 0 (off).");
  REGISTER_CVAR2("r_GraphStyle", &CV_r_graphstyle, 0,VF_NULL,"");
  REGISTER_CVAR2("r_ShowMT", &CV_r_showmt, 0, 0,
    "Shows render multithreading graphs.\n"
    "Usage: r_ShowMT [0/1]\n"
    "Default is 0 (off).");

  REGISTER_CVAR2("r_LogVBuffers", &CV_r_logVBuffers, 0, VF_CHEAT,
		"Logs vertex buffers in memory to 'LogVBuffers.txt'.\n"
		"Usage: r_LogVBuffers [0/1]\n"
		"Default is 0 (off).")->SetOnChangeCallback(GetLogVBuffersStatic);
  REGISTER_CVAR2("r_LogTexStreaming", &CV_r_logTexStreaming, 0,VF_CHEAT,
		"Logs streaming info to Direct3DLogStreaming.txt\n"
		"0: off\n"
		"1: normal\n"
		"2: extended");
  REGISTER_CVAR2("r_LogShaders", &CV_r_logShaders, 0,VF_CHEAT,
    "Logs shaders info to Direct3DLogShaders.txt\n"
    "0: off\n"
    "1: normal\n"
    "2: extended");
  REGISTER_CVAR2("r_Flush", &CV_r_flush, 1,VF_NULL,""); // this causes the game to freeze (infinite loop) - do not use

	REGISTER_CVAR2("r_NoLoadTextures", &CV_r_noloadtextures, 0,VF_CHEAT,"");

  REGISTER_CVAR2("r_OptimisedLightSetup", &CV_r_optimisedlightsetup, 2,VF_NULL,"");

  REGISTER_CVAR2("r_ShadersDebug", &CV_r_shadersdebug, 0, VF_DUMPTODISK,
		"Enable special logging when shaders become compiled\n"
		"Usage: r_ShadersDebug [0/1/2/3]\n"
		" 1 = assembly into directory Main/{Game}/shaders/cache/d3d9\n"
		" 2 = compiler input into directory Main/{Game}/testcg\n"
		" 3 = compiler input with debug information (useful for PIX etc./{Game}/testcg_1pass\n"
		"Default is 0 (off)");
  

    REGISTER_CVAR2("r_ShaderLod", &CV_r_ShaderLod, 0,VF_CHEAT,
    "Enable shader LOD usage - default 0 off - 1 on\n");

#if !defined (XENON) && !defined(PS3)
  REGISTER_CVAR2("r_ShadersXenon", &CV_r_shadersxenon, 0, VF_NULL, "");
  REGISTER_CVAR2("r_ShadersPS3", &CV_r_shadersps3, 0, VF_NULL, "");
  REGISTER_CVAR2("r_ShadersDX9", &CV_r_shadersdx9, 1, VF_NULL, "");
  REGISTER_CVAR2("r_ShadersDX11", &CV_r_shadersdx11, 1, VF_NULL, "");
#endif

  REGISTER_CVAR2("r_ShadersIgnoreIncludesChanging", &CV_r_shadersignoreincludeschanging, 0,VF_NULL,"");
  REGISTER_CVAR2("r_ShadersLazyUnload", &CV_r_shaderslazyunload, 0, 0, "");
  REGISTER_CVAR2("r_ShadersUserFolder", &CV_r_shadersuserfolder, 1,VF_NULL,"");

  REGISTER_CVAR2("r_ShadersPreactivate", &CV_r_shaderspreactivate, 3, VF_DUMPTODISK,"");
  REGISTER_CVAR2("r_ShadersIntCompiler", &CV_r_shadersintcompiler, 1, VF_DUMPTODISK,"");
  REGISTER_CVAR2("r_ShadersNoCompile", &CV_r_shadersnocompile, 0, 0,"");
  REGISTER_CVAR2("r_ShadersNoSources", &CV_r_shadersnosources, 0, 0,"");
	REGISTER_CVAR2("r_ShadersRemoteCompiler", &CV_r_shadersremotecompiler, 0, VF_DUMPTODISK,"");
  REGISTER_CVAR2("r_ShadersAsyncCompiling", &CV_r_shadersasynccompiling, 1, 0,
		"Enable asynchronous shader compiling\n"
		"Usage: r_ShadersAsyncCompiling [0/1/2/3]\n"
		" 0 = off, (stalling) shaders compiling\n"
		" 1 = on, shaders are compiled in parallel, missing shaders are rendered in yellow\n"
		" 2 = on, shaders are compiled in parallel, missing shaders are not rendered\n"
    " 3 = on, shaders are compiled in parallel in precache mode");

	REGISTER_CVAR2("r_CloakLightScale", &CV_r_cloak_light_scale, 1.0f, VF_NULL, "Cloak light scale - default = 1.0");
	
  REGISTER_CVAR2("r_ShadersAsyncMaxThreads", &CV_r_shadersasyncmaxthreads, 1, VF_DUMPTODISK,"");
  REGISTER_CVAR2("r_ShadersCacheOptimiseLog", &CV_r_shaderscacheoptimiselog, 0, VF_NULL,"");
  REGISTER_CVAR2("r_ShadersPrecacheAllLights", &CV_r_shadersprecachealllights, 1,VF_NULL,"");
  REGISTER_CVAR2("r_ShadersDynamicBranching", &CV_r_shadersdynamicbranching, 0,VF_NULL,"");

  REGISTER_CVAR2("r_DebugRenderMode", &CV_r_debugrendermode, 0, VF_CHEAT,"");
  REGISTER_CVAR2("r_DebugRefraction", &CV_r_debugrefraction, 0, VF_CHEAT,
    "Debug refraction usage. Displays red instead of refraction\n"
    "Usage: r_DebugRefraction\n"
    "Default is 0 (off)");
  REGISTER_CVAR2("r_DebugVoxTerrainX", &CV_r_DebugVoxTerrainX, 0, VF_CHEAT,"");
  REGISTER_CVAR2("r_DebugVoxTerrainY", &CV_r_DebugVoxTerrainY, 0, VF_CHEAT,"");
  
  REGISTER_CVAR2("r_MeshPrecache", &CV_r_meshprecache, 1,VF_NULL,"");

  CV_r_excludeshader = REGISTER_STRING("r_ExcludeShader", "0", VF_CHEAT,
		"Exclude the named shader from the render list.\n"
		"Usage: r_ExcludeShader ShaderName\n"
		"Sometimes this is useful when debugging.");
  CV_r_showonlyshader = REGISTER_STRING("r_ShowOnlyShader", "0", VF_CHEAT,
		"Render only the named shader, ignoring all others.\n"
		"Usage: r_ShowOnlyShader ShaderName");

  REGISTER_CVAR2("r_ProfileShaders", &CV_r_profileshaders, 0, VF_CHEAT,
		"Enables display of render profiling information.\n"
		"Usage: r_ProfileShaders [0/1]\n"
		"Default is 0 (off). Set to 1 to display profiling\n"
		"of rendered shaders.");
  REGISTER_CVAR2("r_ProfileShadersSmooth", &CV_r_ProfileShadersSmooth, 4, VF_CHEAT,
    "Smooth time information.\n"
    "Usage: r_ProfileShadersSmooth [0-10]");
  REGISTER_CVAR2("r_ProfileShadersGroupByName", &CV_r_ProfileShadersGroupByName, 1, VF_CHEAT,
    "Group items by name ignoring RT flags.\n"
    "Usage: r_ProfileShaders [0/1]");
  
  REGISTER_CVAR2("r_EnvCMWrite", &CV_r_envcmwrite, 0, 0,
		"Writes cube-map textures to disk.\n"
		"Usage: r_EnvCMWrite [0/1]\n"
		"Default is 0 (off). The textures are written to 'Cube_posx.jpg'\n"
		"'Cube_negx.jpg',...,'Cube_negz.jpg'. At least one of the real-time\n"
		"cube-map shaders should be present in the current scene.");
  
  REGISTER_CVAR2("r_EnvCMResolution", &CV_r_envcmresolution, 1, VF_DUMPTODISK,
		"Sets resolution for target environment cubemap, in pixels.\n"
		"Usage: r_EnvCMResolution #\n"
		"where # represents:\n"
		"	0: 64\n"
		"	1: 128\n"
		"	2: 256\n"
		"Default is 2 (256 by 256 pixels).");
  
  REGISTER_CVAR2("r_EnvTexResolution", &CV_r_envtexresolution, 3, VF_DUMPTODISK,
		"Sets resolution for 2d target environment texture, in pixels.\n"
		"Usage: r_EnvTexResolution #\n"
		"where # represents:\n"
		"	0: 64\n"
		"	1: 128\n"
		"	2: 256\n"
		"	3: 512\n"
		"Default is 3 (512 by 512 pixels).");
  
  REGISTER_CVAR2("r_WaterUpdateDistance", &CV_r_waterupdateDistance, 2.0f, 0,"");
  
  REGISTER_CVAR2("r_WaterUpdateFactor", &CV_r_waterupdateFactor, 0.01f, VF_DUMPTODISK,
		"Distance factor for water reflected texture updating.\n"
		"Usage: r_WaterUpdateFactor 0.01\n"
		"Default is 0.01. 0 means update every frame");
  
  REGISTER_CVAR2("r_EnvLCMupdateInterval", &CV_r_envlcmupdateinterval, 0.1f, VF_DUMPTODISK,
					"LEGACY - not used");
//		"Sets the interval between environmental cube map texture updates.\n"
//		"Usage: r_EnvCMupdateInterval 0.1\n"
//		"Default is 0.1.");
  REGISTER_CVAR2("r_EnvCMupdateInterval", &CV_r_envcmupdateinterval, 0.04f, VF_DUMPTODISK,
		"Sets the interval between environmental cube map texture updates.\n"
    "Usage: r_EnvCMupdateInterval #\n"
		"Default is 0.1.");
  REGISTER_CVAR2("r_EnvTexUpdateInterval", &CV_r_envtexupdateinterval, 0.001f, VF_DUMPTODISK,
		"Sets the interval between environmental 2d texture updates.\n"
		"Usage: r_EnvTexUpdateInterval 0.001\n"
		"Default is 0.001.");
  
  REGISTER_CVAR2("r_WaterReflections", &CV_r_waterreflections, 1, VF_DUMPTODISK,
		"Toggles water reflections.\n"
		"Usage: r_WaterReflections [0/1]\n"
		"Default is 1 (water reflects).");

  REGISTER_CVAR2("r_WaterReflectionsMGPU", &CV_r_waterreflections_mgpu, 1, VF_DUMPTODISK,
    "Toggles water reflections.multi-gpu support\n"
    "Usage: r_WaterReflectionsMGPU [0/1/2]\n"
    "Default is 0 (single render update), 1 (multiple render updates)");

  REGISTER_CVAR2("r_WaterReflectionsQuality", &CV_r_waterreflections_quality, 0, VF_DUMPTODISK,
		"Activates water reflections quality setting.\n"
		"Usage: r_WaterReflectionsQuality [0/1/2/3]\n"
		"Default is 0 (terrain only), 1 (terrain + particles), 2 (terrain + particles + brushes), 3 (everything)");

  REGISTER_CVAR2("r_WaterReflectionsUseMinOffset", &CV_r_waterreflections_use_min_offset, 1, VF_DUMPTODISK,
    "Activates water reflections use min distance offset.");

  REGISTER_CVAR2("r_WaterReflectionsMinVisiblePixelsUpdate", &CV_r_waterreflections_min_visible_pixels_update, 0.05f, VF_DUMPTODISK,
    "Activates water reflections if visible pixels above a certain threshold.");

  REGISTER_CVAR2("r_WaterReflectionsMinVisUpdateFactorMul", &CV_r_waterreflections_minvis_updatefactormul, 20.0f, VF_DUMPTODISK,
    "Activates update factor multiplier when water mostly occluded.");  
  REGISTER_CVAR2("r_WaterReflectionsMinVisUpdateDistanceMul", &CV_r_waterreflections_minvis_updatedistancemul, 10.0f, VF_DUMPTODISK,
    "Activates update distance multiplier when water mostly occluded.");  

  REGISTER_CVAR2("r_WaterCaustics", &CV_r_watercaustics,1, 0,
    "Toggles under water caustics.\n"
    "Usage: r_WaterCaustics [0/1]\n"
    "Default is 1 (enabled).");  

  REGISTER_CVAR2("r_WaterCausticsDistance", &CV_r_watercausticsdistance,100.0f, 0,
    "Toggles under water caustics max distance.\n"
    "Usage: r_WaterCausticsDistance\n"
    "Default is 100.0 meters");  
 
	REGISTER_CVAR2("r_WaterCausticsDeferred", &CV_r_watercausticsdeferred,0, 0,
		"Toggles under water caustics deferred pass.\n"
		"Usage: r_WaterCausticsDeferred [0/1/2]\n"
		"Default is 0 (disabled). 1 - enables. 2 - enables with stencil pre-pass");  
	
  REGISTER_CVAR2("r_WaterGodRays", &CV_r_water_godrays, 1, 0,
    "Enables under water god rays.\n"
    "Usage: r_WaterGodRays [0/1]\n"
    "Default is 1 (enabled).");  

	REGISTER_CVAR2("r_WaterRipples", &CV_r_water_ripples, 0, 0,
		"Enables water interaction/ripples.\n"
		"Usage: r_WaterRipples [0/1]\n"
		"Default is 1 (enabled).");  
	
  REGISTER_CVAR2("r_Reflections", &CV_r_reflections, 1, VF_DUMPTODISK,
    "Toggles reflections.\n"
    "Usage: r_Reflections [0/1]\n"
    "Default is 1 (reflects).");

  REGISTER_CVAR2("r_ReflectionsOffset", &CV_r_waterreflections_offset, 0.0f,VF_NULL,"");
    
  REGISTER_CVAR2("r_ReflectionsQuality", &CV_r_reflections_quality, 3, VF_DUMPTODISK,
    "Toggles reflections quality.\n"
    "Usage: r_ReflectionsQuality [0/1/2/3]\n"
    "Default is 0 (terrain only), 1 (terrain + particles), 2 (terrain + particles + brushes), 3 (everything)");
    
  REGISTER_CVAR2("r_DetailTextures", &CV_r_detailtextures, 1, VF_DUMPTODISK,
		"Toggles detail texture overlays.\n"
		"Usage: r_DetailTextures [0/1]\n"
		"Default is 1 (detail textures on).");

  REGISTER_CVAR2("r_ReloadShaders", &CV_r_reloadshaders, 0, VF_CHEAT,
		"Reloads shaders.\n"
		"Usage: r_ReloadShaders [0/1]\n"
		"Default is 0. Set to 1 to reload shaders.");

  REGISTER_CVAR2("r_DetailNumLayers", &CV_r_detailnumlayers, 2, VF_DUMPTODISK,
		"Sets the number of detail layers per surface.\n"
		"Usage: r_DetailNumLayers 2\n"
		"Default is 2.");
  REGISTER_CVAR2("r_DetailScale", &CV_r_detailscale, 8.0f, 0,
		"Sets the default scaling for detail overlays.\n"
		"Usage: r_DetailScale 8\n"
		"Default is 8. This scale applies only if the object's\n"
		"detail scale was not previously defined (in MAX).");
  REGISTER_CVAR2("r_DetailDistance", &CV_r_detaildistance, 6.0f, VF_DUMPTODISK,
		"Distance used for per-pixel detail layers blending.\n"
    "Usage: r_DetailDistance (1-20)\n"
		"Default is 6.");
  
  REGISTER_CVAR2("r_TexBindMode", &CV_r_texbindmode, 0, VF_CHEAT, "");
  REGISTER_CVAR2("r_NoDrawShaders", &CV_r_nodrawshaders, 0, VF_CHEAT,
		"Disable entire render pipeline.\n"
		"Usage: r_NoDrawShaders [0/1]\n"
		"Default is 0 (render pipeline enabled). Used for debugging and profiling.");
  REGISTER_CVAR2("r_NoDrawNear", &CV_r_nodrawnear, 0, 0,
		"Disable drawing of near objects.\n"
		"Usage: r_NoDrawNear [0/1]\n"
		"Default is 0 (near objects are drawn).");
  REGISTER_CVAR2("r_DrawNearZRange", &CV_r_DrawNearZRange, 0.1f, 0,
    "Default is 0.1.");
  REGISTER_CVAR2("r_DrawNearFarPlane", &CV_r_DrawNearFarPlane, 40.0f, 0,
    "Default is 40.");
	REGISTER_CVAR2("r_DrawNearFoV", &CV_r_drawnearfov, 60.0f, 0,
		"Sets the FoV for drawing of near objects.\n"
		"Usage: r_DrawNearFoV [n]\n"
		"Default is 60.");

  REGISTER_CVAR2("r_Flares", &CV_r_flares, 1, VF_DUMPTODISK,
		"Toggles sunlight lens flare effect.\n"
		"Usage: r_Flares [0/1]\n"
		"Default is 1 (on).");

  REGISTER_CVAR2("r_Gamma", &CV_r_gamma, 1.0f, VF_DUMPTODISK,
		"Adjusts the graphics card gamma correction (fast, needs hardware support, affects also HUD and desktop)\n"
		"Usage: r_Gamma 1.0\n"
		"1 off (default), try values like 1.6 or 2.2");
  REGISTER_CVAR2("r_Brightness", &CV_r_brightness, 0.5f, VF_DUMPTODISK,
		"Sets the display brightness.\n"
		"Usage: r_Brightness 0.5\n"
		"Default is 0.5.");
  REGISTER_CVAR2("r_Contrast", &CV_r_contrast, 0.5f, VF_DUMPTODISK,
		"Sets the display contrast.\n"
		"Usage: r_Contrast 0.5\n"
		"Default is 0.5.");
	
  REGISTER_CVAR2("r_NoHWGamma", &CV_r_nohwgamma, 0, VF_DUMPTODISK,
		"Sets renderer to ignore hardware gamma correction.\n"
		"Usage: r_NoHWGamma [0/1]\n"
		"Default is 0 (allow hardware gamma correction).");

  REGISTER_CVAR2("r_Scissor", &CV_r_scissor, 1, 0, "Enables scissor test");
  REGISTER_CVAR2("r_Coronas", &CV_r_coronas, 1, VF_DUMPTODISK,
		"Toggles light coronas around light sources.\n"
		"Usage: r_Coronas [0/1]"
		"Default is 1 (on).");
  REGISTER_CVAR2("r_CoronaFade", &CV_r_coronafade, 0.5f, 0,
		"Time fading factor of the light coronas.\n"
		"Usage: r_CoronaFade 0.5"
		"Default is 0.5.");
  REGISTER_CVAR2("r_CoronaSizeScale", &CV_r_coronasizescale, 1.0f,VF_NULL,"");
  REGISTER_CVAR2("r_CoronaColorScale", &CV_r_coronacolorscale, 1.0f, 0,"");

  REGISTER_CVAR2("r_CullByClipPlanes", &CV_r_cullbyclipplanes, 1,VF_NULL,"");

  REGISTER_CVAR2("r_wireframe", &CV_r_wireframe, R_SOLID_MODE, VF_CHEAT,"");
	REGISTER_CVAR2("r_GetScreenShot", &CV_r_GetScreenShot, 0, 0,
     "To capture one screenshot (variable is set to 0 after capturing)\n"
     "0=do not take a screenshot (default), 1=save a screenshot (together with .HDR if enabled), 2=save a screenshot");

  REGISTER_CVAR2("r_Character_NoDeform", &CV_r_character_nodeform, 0,VF_NULL,"");

  REGISTER_CVAR2("r_Log", &CV_r_log, 0, VF_CHEAT,
		"Logs rendering information to Direct3DLog.txt.\n"
		"Use negative values to log a single frame.\n"
		"Usage: r_Log +/-[0/1/2/3/4]\n"
		"	1: Logs a list of all shaders without profile info.\n"
		"	2: Log contains a list of all shaders with profile info.\n"
		"	3: Logs all API function calls.\n"
		"	4: Highly detailed pipeline log, including all passes,\n"
		"			states, lights and pixel/vertex shaders.\n"
		"Default is 0 (off). Use this function carefully, because\n"
		"log files grow very quickly.");

  REGISTER_CVAR2("r_LogVidMem", &CV_r_logVidMem, 0, VF_CHEAT,
    "Logs vid mem information to VidMemLog.txt.");
  
  REGISTER_CVAR2("r_Stats", &CV_r_stats, 0, VF_CHEAT,
		"Toggles render statistics.\n"
    "0=disabled,\n"
    "1=global render stats,\n"
    "2=print shaders for selected object,\n"
    "6=display per-instance drawcall count,\n"
    "11=print info about used RT's (switches),\n"
    "12=print info about used unique RT's,\n"
    "13=print info about cleared RT's\n"
		"Usage: r_Stats [0/1/2/3/6/11/12/13]");
  REGISTER_CVAR2("r_VSync", &CV_r_vsync, 0, VF_RESTRICTEDMODE|VF_DUMPTODISK,
		"Toggles vertical sync.\n"
		"Usage: r_VSync [0/1]");
  REGISTER_CVAR2("r_PredicatedTiling", &CV_r_predicatedtiling, 0, VF_REQUIRE_APP_RESTART,
		"Toggles predicated tiling mode (X360 only)\n"
    "Usage: r_PredicatedTiling [0/1]");
  REGISTER_CVAR2("r_testSplitScreen", &CV_r_testSplitScreen, 0, 0,
    "Toggles split screen test mode\n"
    "Usage: r_testSplitScreen [0/1]");
	REGISTER_CVAR2("r_SplitScreenActive", &CV_r_SplitScreenActive, 0, 0,
		"Activates split screen mode\n"
		"Usage: r_SplitScreenActive [0/1]");
	REGISTER_CVAR2("r_MeasureOverdraw", &CV_r_measureoverdraw, 0, VF_CHEAT,
		"0=off, 1=activate special rendering mode that visualize the rendering cost per pixel by colour\n"
		"Usage: r_MeasureOverdraw [0/1]");
  REGISTER_CVAR2("r_MeasureOverdrawScale", &CV_r_measureoverdrawscale, 1.5f, VF_CHEAT,"");
  REGISTER_CVAR2("r_PrintMemoryLeaks", &CV_r_printmemoryleaks, 0,VF_NULL,"");
  REGISTER_CVAR2("r_ReleaseAllResourcesOnExit", &CV_r_releaseallresourcesonexit, 0,VF_NULL,"");

  REGISTER_CVAR2("r_VegetationSpritesDebug",&CV_r_VegetationSpritesGenDebug,0,VF_NULL,"");
  REGISTER_CVAR2("r_VegetationSpritesNoBend",&CV_r_VegetationSpritesNoBend,2,VF_NULL,"");
  REGISTER_CVAR2("r_ZPassOnly",&CV_r_ZPassOnly,0,VF_CHEAT,"");
  REGISTER_CVAR2("r_VegetationSpritesNoGen",&CV_r_VegetationSpritesNoGen,0,VF_NULL,"");
  REGISTER_CVAR2("r_VegetationSpritesGenAlways",&CV_r_VegetationSpritesGenAlways,0,VF_NULL,"");
  REGISTER_CVAR2("r_VegetationSpritesIgnoreAngle",&CV_r_VegetationSpritesIgnoreAngle,0,VF_NULL,"");
  REGISTER_CVAR2("r_VegetationSpritesTexRes",&CV_r_VegetationSpritesTexRes,64,VF_NULL,"");
  REGISTER_CVAR2("r_VegetationAlphaTestOnly",&CV_r_VegetationAlphaTestOnly,0,VF_NULL,"");

	REGISTER_CVAR2("r_ShowVideoMemoryStats", &CV_r_ShowVideoMemoryStats, 0,VF_NULL,"");
	REGISTER_CVAR2("r_ShowRenderTarget", &CV_r_ShowRenderTarget, 0, VF_CHEAT,
		"Displays special render targets - for debug purpose\n"
		"When 8-bit content is displayed, banding may occur if sRGB back buffer writes are enabled\n"
		"Usage: r_ShowRenderTarget [0=off/1/2/3/4/5/6/7/8/9/...]\n"
		"1: m_Text_ZTarget\n"
		"2: m_Text_HDRTarget\n"
		"3: m_Text_ScreenShadowMap[0]\n"
		"4: m_Text_ScreenShadowMap[1]\n"
		"5: m_Text_ScreenShadowMap[2]\n"
		"6: gTexture\n"
		"7: gTexture2\n"
		"8: m_Text_ScatterLayer\n"
		"9: pEnvTex->m_pTex->m_pTexture\n"
		"10: m_Text_LightInfo[0]\n"
    "11: SSAO render target\n"    
    "12: m_Text_BackBufferScaled[0] (Current 2x smaller post effects backbuffer) \n"
    "13: m_Text_BackBufferScaled[1] (Current 4x smaller post effects backbuffer) \n"
    "14: m_Text_BackBufferScaled[2] (Current 8x smaller post effects backbuffer) \n"
    "15: Current custom render target (reflections/etc) \n"
    "16: Downscaled depth target for SSAO\n"
    "17: m_Text_SceneNormalsMap (View space normals target)\n"
    "18: Deferred Indirect lighting accumulation (wip research)\n"
    "19: m_Text_SceneTarget\n"
		"20: Irradiance volume visualization\n"
		"26: Stereo left and right buffers\n");

	// show texture debug routine + auto completion
	CV_r_ShowTexture = REGISTER_STRING("r_ShowTexture", "", VF_CHEAT, "Displays loaded texture - for debug purpose\n");
	gEnv->pConsole->RegisterAutoComplete( "r_ShowTexture",&g_TextureNameAutoComplete );

	REGISTER_CVAR2("r_ShowRenderTarget_FullScreen", &CV_r_ShowRenderTarget_FullScreen, 0,VF_CHEAT,"");
  REGISTER_CVAR2("r_ShowLightBounds", &CV_r_ShowLightBounds, 0, VF_CHEAT,
    "Display light bounds - for debug purpose\n"
    "Usage: r_ShowLightBounds [0=off/1=on]");
	REGISTER_CVAR2("r_MergeRenderChunksForDepth", &CV_r_MergeRenderChunksForDepth, 0,VF_NULL,"");

	REGISTER_CVAR2("r_UseGSParticles", &CV_r_UseGSParticles, 1, 0, 
		"Toggles use of geometry shader particles (DX11 only, changing at runtime is supported).\n"
		"Usage: r_UseGSParticles [0/1=default]");
	REGISTER_CVAR2("r_Force3DcEmulation", &CV_r_Force3DcEmulation,2,0, 
		"Specifies renderer behavior for the 3Dc (compressed normal maps) emulation (DX9 only).\n"
		"(Emulation: DXT5, less quality but same memory requirements)\n"
		"0=use only if driver doesn't support 3Dc\n"
		"1=enforce on any hardware\n"
		"2=use only if hardware doesn't support 3Dc\n"
		"Usage: r_Force3DcEmulation [0/1/2]")->SetOnChangeCallback(ChangeForce3DcEmulation);
	
	REGISTER_CVAR2("r_ZFightingDepthScale", &CV_r_ZFightingDepthScale, 0.995f, VF_CHEAT, "Controls anti z-fighting measures in shaders (scaling homogeneous z)." );
	REGISTER_CVAR2("r_ZFightingExtrude", &CV_r_ZFightingExtrude, 0.001f, VF_CHEAT, "Controls anti z-fighting measures in shaders (extrusion along normal in world units)." );

	REGISTER_CVAR2("r_UseSRGB", &CV_r_useSRGB, 1, VF_REQUIRE_APP_RESTART, "Enables sRGB texture reads / framebuffer writes.\n"
		"Usage: r_UseSRGB [0=off/1=lighting is computed in linear space]");
	ICVar* pCV_r_TexelsPerMeter = REGISTER_CVAR2("r_TexelsPerMeter", &CV_r_TexelsPerMeter, 0, 0, 
		"Enables visualization of the color coded \"texels per meter\" ratio for objects in view.\n"
		"The checkerboard pattern displayed represents the mapping of the assigned diffuse\n"
		"texture onto the object's uv space. One block in the pattern represents 8x8 texels.\n"
		"Usage: r_TexelsPerMeter [n] (where n is the desired number of texels per meter; 0 = off)");
	if (pCV_r_TexelsPerMeter)
		pCV_r_TexelsPerMeter->SetOnChangeCallback(OnChange_CV_r_TexelsPerMeter);

	REGISTER_CVAR2("r_Stereo", &CV_r_Stereo, 0, VF_DUMPTODISK|VF_REQUIRE_APP_RESTART,
		"Sets stereo rendering mode. Stereo needs to be enabled before application start and works only\n"
		"with d3d9 in fullscreen mode.\n"
		"Usage: r_Stereo [0=off/1/2]\n"
		"1: Generic dualhead output\n"
		"2: iZ3D dualhead output"
		"3: Debug mode, render views as usual, no need for fullscreen, but only first view visible - use r_ShowRenderTarget for debugging additional views");
	REGISTER_CVAR2("r_StereoEyeDist", &CV_r_StereoEyeDist, 0.06f, VF_DUMPTODISK,
		"Distance between eyes used for stereo (interocular distance)");
	REGISTER_CVAR2("r_StereoScreenDist", &CV_r_StereoScreenDist, 60.0f, VF_DUMPTODISK,
		"Distance to plane where stereo parallax converges to zero");

  REGISTER_CVAR2("r_ConditionalRendering", &CV_r_ConditionalRendering, 0, VF_CHEAT, "Enables conditional rendering .");










#if !defined (XENON) && !defined(PS3) && !defined(NULL_RENDERER)
  REGISTER_COMMAND("r_PrecacheShaders", &ShadersPrecache,VF_NULL,"");
	REGISTER_COMMAND("r_PrecacheShaderList", &ShadersPrecacheList,VF_NULL,"");
  REGISTER_COMMAND("r_StatsShaders", &ShadersStats,VF_NULL,"");
  REGISTER_COMMAND("r_StatsShaderList", &ShadersStatsList,VF_NULL,"");
  REGISTER_COMMAND("r_OptimiseShaders", &ShadersOptimise,VF_NULL,"");
  REGISTER_COMMAND("r_MergeShaders", &ShadersMerge,VF_CHEAT,"");
  REGISTER_COMMAND("r_PrecacheShadersLevels", &ShadersPrecacheLevels,VF_NULL,"");
#endif

	REGISTER_CVAR2("r_TextureCompressor", &CV_r_TextureCompressor, 1, VF_DUMPTODISK,
		"Defines which texture compressor is used (fallback is DirectX)\n"
		"Usage: r_TextureCompressor [0/1]\n"
		"0 uses nvDXT, 1 uses Squish if possible");

	REGISTER_CVAR2("r_FogDepthTest", &CV_r_FogDepthTest, 0, 0,
		"Set depth test threshold (in clip space) for volumetric fog pass.\n"
		"Usage: r_FogDepthTest [0..1]");

	REGISTER_CVAR2("r_FogColorGradient", &CV_r_FogColorGradient, 0, 0,
		"Enables a user controllable color gradient for global volumetric fog.\n"
		"Usage: r_FogColorGradient [0/1]");

	REGISTER_CVAR2("r_FlashMatTexResQuality", &CV_r_FlashMatTexResQuality, 1.0f, 0,
		"Texture resolution quality of flash materials.\n"
		"Only used if flash asset is tagged CE_HQR!");

	// more details: http://en.wikipedia.org/wiki/Overscan
	// Microsoft's Xbox game developer guidelines recommend using 85 percent of the screen width and height,[7] or a title safe area of 7.5% per side.
	REGISTER_COMMAND("r_OverscanBorders", &cmd_OverscanBorders, VF_NULL,
		"Changes the size of the overscan borders for the left/right and top/bottom\n"
		"of the screen for adjusting the title safe area. This is for logo placements\n"
		"and text printout to account for the TV overscan and is mostly needed for consoles.\n"
		"If only one value is specified, the overscan borders for left/right and top/bottom\n"
		"are set simultaneously, but you may also specify different percentages for left/right\n"
		"and top/bottom.\n"
		"Usage: r_OverscanBorders [0..25]\n"
		"       r_OverscanBorders [0..25] [0..25]\n"
		"Default is 0=off, >0 defines the size of the overscan borders for left/right\n"
		"or top/bottom as percentages of the whole screen size (e.g. 7.5).");
	
	// ----------------------------------------------------------------









  m_cClearColor = ColorF(0,0,0,128.0f/255.0f);		// 128 is default GBuffer value
  m_LogFile = NULL;
	m_pDefaultFont = NULL;
  m_TexGenID = 1;
  m_VSync = CV_r_vsync;
  m_Features = 0;
#if !defined(XENON) && !defined(PS3)
	m_bNVLibInitialized=false;
	m_bDriverHasActiveMultiGPU=false;
#endif

  //init_math();
  
  m_RP.m_TI[0].m_nFrameID = (unsigned short)-2;
  m_RP.m_TI[1].m_nFrameID = (unsigned short)-2;

  m_bPauseTimer=0;
  m_fPrevTime=-1.0f;

//  m_RP.m_ShaderCurrTime = 0.0f;

  m_CurFontColor = Col_White;

  m_bUseHWSkinning = CV_r_usehwskinning != 0;
  m_bWaterCaustics = CV_r_watercaustics != 0;

	m_bSwapBuffers = true;
  m_RP.m_TI[0].m_FS.m_bEnable = true;
  m_RP.m_TI[1].m_FS.m_bEnable = true;

#if defined(_DEBUG) && defined(WIN32)
  if (CV_r_printmemoryleaks)
  {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  }
#endif

  m_pREPostProcess=0;
  m_bUseZpass = CV_r_usezpass != 0;  
	m_nCloudShadowTexId = 0;
  m_vCloudShadowSpeed.Set(0,0,0);
	m_pSkyLightRenderParams = 0;
	m_RP.m_fogVolumeContibutions[0].reserve( 1024 );
#if !defined(STRIP_RENDER_THREAD)
  m_RP.m_fogVolumeContibutions[1].reserve( 1024 );
#endif
#ifndef EXCLUDE_SCALEFORM_SDK
	m_pSFDrawParams = 0;
#endif

  m_nGPUs = 1;

  //assert(!(FOB_MASK_AFFECTS_MERGING & 0xffff));
  //assert(sizeof(CRenderObject) == 256);
#if !defined(WIN64) && !defined(LINUX64)
  STATIC_CHECK(sizeof(CRenderObject) == 128, CRenderObject);
#endif
  STATIC_CHECK(!(FOB_MASK_AFFECTS_MERGING & 0xffff), FOB_MASK_AFFECTS_MERGING);

	if (!g_pSDynTexture_PoolAlloc) 
		g_pSDynTexture_PoolAlloc = new SDynTexture_PoolAlloc;

  //m_RP.m_VertPosCache.m_nBufSize = 500000 * sizeof(Vec3);
  //m_RP.m_VertPosCache.m_pBuf = new byte [gRenDev->m_RP.m_VertPosCache.m_nBufSize];

  m_pDefaultMaterial = NULL;
  m_pTerrainDefaultMaterial = NULL;

	m_ViewMatrix.SetIdentity();
	m_CameraMatrix.SetIdentity();
	m_CameraZeroMatrix[0].SetIdentity();
  m_CameraZeroMatrix[1].SetIdentity();
  m_CameraMatrixPrev[0].SetIdentity();
	m_CameraMatrixPrev[1].SetIdentity();
	m_ProjMatrix.SetIdentity();
	m_TranspOrigCameraProjMatrix.SetIdentity();
  m_CameraProjMatrix.SetIdentity();  
	m_CameraProjZeroMatrix.SetIdentity();
	m_InvCameraProjMatrix.SetIdentity();
  m_IdentityMatrix.SetIdentity();

  m_RP.m_oldOcclusionCamera.SetIdentity();
  m_RP.m_newOcclusionCamera.SetIdentity();

	for (int i = 0; i < SIZEOF_ARRAY(m_TempMatrices); i++)
		for (int j = 0; j < SIZEOF_ARRAY(m_TempMatrices[0]); j++)
			m_TempMatrices[i][j].SetIdentity();

  CParserBin::m_bParseFX = true;
  //CParserBin::m_bEmbeddedSearchInfo = false;
#ifdef SHADERS_EDITABLE
  CParserBin::m_bEditable = true;
#else
  if (iSystem->IsEditor())
    CParserBin::m_bEditable = true;
#endif

	CV_e_DebugTexelDensity = 0;

	m_pDebugRenderNode = NULL;

  m_pRT = new SRenderThread;
	m_pRT->Start();

	// on console some float values in vertex formats can be 16 bit
	iLog->Log("CRenderer sizeof(Vec2f16)=%d sizeof(Vec3f16)=%d",sizeof(Vec2f16),sizeof(Vec3f16));
}

CRenderer::~CRenderer()
{
	// Create Post Process Effects Mgr.
	PostEffectMgr().Release();

	delete g_pSDynTexture_PoolAlloc;
	g_pSDynTexture_PoolAlloc = NULL;
	gRenDev = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::PostInit()
{
	// Create Post Process Effects Mgr.
	PostEffectMgr().Create();

	//////////////////////////////////////////////////////////////////////////
	// Load internal renderer font.
	//////////////////////////////////////////////////////////////////////////
	if (gEnv->pCryFont)
	{
		m_pDefaultFont = gEnv->pCryFont->NewFont("renderer_internal");
		if (!m_pDefaultFont->Load(RENDERER_DEFAULT_FONT) )
		{
			CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_ERROR,"Error loading font: %s",RENDERER_DEFAULT_FONT );
			m_pDefaultFont = 0;
		}

	}
	//////////////////////////////////////////////////////////////////////////
}

void CRenderer::Release()
{
  delete this;
}

//////////////////////////////////////////////////////////////////////
void CRenderer::AddListener(IRendererEventListener *pRendererEventListener)
{
	stl::push_back_unique(m_listRendererEventListeners,pRendererEventListener);
}

//////////////////////////////////////////////////////////////////////
void CRenderer::RemoveListener(IRendererEventListener *pRendererEventListener)
{
	stl::find_and_erase(m_listRendererEventListeners,pRendererEventListener);
}

//////////////////////////////////////////////////////////////////////
/*bool CRenderer::FindImage(CImage *image)
{
	for (ImageIt i=m_imageList.begin();i!=m_imageList.end();i++)
	{
		CImage *ci=(*i);

		if (ci==image) 
			return (true);
	} //i

	return (false);
} */

//////////////////////////////////////////////////////////////////////
/*CImage *CRenderer::FindImage(const char *filename)
{

	ImageIt istart=m_imageList.begin();
	ImageIt iend=m_imageList.end();

	for (ImageIt i=m_imageList.begin();i!=iend;i++)
	{
		CImage *ci=(*i);

		if (stricmp(ci->GetName(),filename)==0) 
			return (ci);
	} //i

	return (NULL);
} */

//////////////////////////////////////////////////////////////////////
/*void CRenderer::AddImage(CImage *image)
{
	m_imageList.push_back(image);
} */

//////////////////////////////////////////////////////////////////////
//void CRenderer::ShowFps(const char *command/* =NULL */)
/*{
	if (!command) 
		return;
	if (stricmp(command,"true")==0) 
		m_showfps=true;
	else
	if (stricmp(command,"false")==0) 
		m_showfps=false;	
	else
	  iConsole->Help("ShowFps");
} */

//////////////////////////////////////////////////////////////////////
void CRenderer::TextToScreenColor(int x, int y, float r, float g, float b, float a, const char * format, ...)
{
//  if(!cVars->e_text_info)
  //  return;

  char buffer[512];
  va_list args;
  va_start(args, format);
	if (vsnprintf(buffer,sizeof(buffer),format, args) == -1)
		buffer[sizeof(buffer)-1] = 0;
  va_end(args);

  WriteXY((int)(8*x), (int)(6*y), 1, 1, r,g,b,a, buffer);
}

//////////////////////////////////////////////////////////////////////
void CRenderer::TextToScreen(float x, float y, const char * format, ...)
{
//  if(!cVars->e_text_info)
  //  return;

  char buffer[512];
  va_list args;
  va_start(args, format);
	if (vsnprintf(buffer,sizeof(buffer),format, args) == -1)
		buffer[sizeof(buffer)-1] = 0;
  va_end(args);

  WriteXY((int)(8*x), (int)(6*y), 1, 1, 1,1,1,1, buffer);
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::Draw2dText( float posX,float posY,const char *szText,SDrawTextInfo &ti )
{
  // Check for the presence of a D3D device
  if (!iSystem)
    return;

	if (!m_pDefaultFont)
		return;
	IFFont *pFont = m_pDefaultFont;

  float r = CLAMP( ti.color[0], 0.0f, 1.0f);
  float g = CLAMP( ti.color[1], 0.0f, 1.0f);
  float b = CLAMP( ti.color[2], 0.0f, 1.0f);
  float a = CLAMP( ti.color[3], 0.0f, 1.0f);

  pFont->SetColor(ColorF(r,g,b,a));
	pFont->SetCharWidthScale(1);

	pFont->UseFrame((ti.flags&eDrawText_Framed) != 0);

	if (ti.flags & eDrawText_Monospace)
	{
		if (ti.flags & eDrawText_FixedSize)
			pFont->SetSizeIn800x600(false);
		pFont->SetSize( vector2f(UIDRAW_TEXTSIZEFACTOR*ti.xscale,UIDRAW_TEXTSIZEFACTOR*ti.yscale) );
		pFont->SetCharWidthScale(0.5f);
		pFont->SetProportional(false);

		if (ti.flags & eDrawText_800x600)
		{
			posX = ScaleCoordX(posX);
			posY = ScaleCoordY(posY);
		}
	}
	else if (ti.flags & eDrawText_FixedSize)
  {
		pFont->SetSizeIn800x600(false);
		pFont->SetSize( vector2f(UIDRAW_TEXTSIZEFACTOR*ti.xscale,UIDRAW_TEXTSIZEFACTOR*ti.yscale) );
		pFont->SetProportional(true);

		if (ti.flags & eDrawText_800x600)
		{
			posX = ScaleCoordX(posX);
			posY = ScaleCoordY(posY);
		}
  }
  else
  {
    pFont->SetSizeIn800x600(true);
    pFont->SetProportional(false);
    pFont->SetCharWidthScale(0.5f);
    pFont->SetSize(vector2f(UIDRAW_TEXTSIZEFACTOR*ti.xscale,UIDRAW_TEXTSIZEFACTOR*ti.yscale));
  }

	// align left/right/center
	if (ti.flags & eDrawText_Center)
		posX -= pFont->GetTextSize(szText).x * 0.5f;
	else if (ti.flags & eDrawText_Right)
		posX -= pFont->GetTextSize(szText).x;

  pFont->SetColor(ColorF(r,g,b,a));
  
	pFont->DrawString(posX,posY,szText);
}

void CRenderer::PrintToScreen(float x, float y, float size, const char *buf)
{
  SDrawTextInfo ti;
  ti.xscale = size*0.5f/8;
  ti.yscale = size*1.f/8;
  ti.color[0] = 1; ti.color[1] = 1; ti.color[2] = 1; ti.color[3] = 1;
	ti.flags = eDrawText_800x600|eDrawText_2D;
  Draw2dText( x,y,buf,ti );
}

void CRenderer::WriteXY( int x, int y, float xscale, float yscale, float r, float g, float b, float a, const char *format, ...)
{
  //////////////////////////////////////////////////////////////////////
  // Write a string to the screen
  //////////////////////////////////////////////////////////////////////

  va_list args;
  char buffer[512];

  // Check for the presence of a D3D device
  // Format the string
  va_start(args, format);
	if (vsnprintf(buffer,sizeof(buffer),format, args) == -1)
		buffer[sizeof(buffer)-1] = 0;
  va_end(args);

  SDrawTextInfo ti;
  ti.xscale = xscale;
  ti.yscale = yscale;
  ti.color[0] = r;
  ti.color[1] = g;
  ti.color[2] = b;
  ti.color[3] = a;
	ti.flags = eDrawText_800x600|eDrawText_2D;
  Draw2dText( (float)x,(float)y,buffer,ti );
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::DrawTextQueued( Vec3 pos,SDrawTextInfo &ti,const char *text )
{
	int nT = m_pRT->GetThreadList();
	if (text && !GetISystem()->IsDedicated())
	{
		// ti.yscale is currently ignored, input struct can be refactored

		ColorB col(ColorF(ti.color[0],ti.color[1],ti.color[2],ti.color[3]));

		m_TextMessages[nT].PushEntry_Text(pos,col,ti.xscale,ti.flags,text);
	}
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::DrawTextQueued( Vec3 pos,SDrawTextInfo &ti,const char *format,va_list args )
{
	int nT = m_pRT->GetThreadList();
	if (format && !GetISystem()->IsDedicated())
	{
		char str[256];

		vsnprintf_s(str,sizeof(str),sizeof(str)-1,format, args);
		str[sizeof(str)-1] = 0;

		// ti.yscale is currently ignored, input struct can be refactored

		ColorB col(ColorF(ti.color[0],ti.color[1],ti.color[2],ti.color[3]));

		m_TextMessages[nT].PushEntry_Text(pos,col,ti.xscale,ti.flags,str);
	}
}
//////////////////////////////////////////////////////////////////////////
bool CRenderer::CalculateTextSize(Vec2& rOutputValue, SDrawTextInfo &ti,const char *format,va_list args,const bool bASCIIMultiLine )
{
	char str[256];

	vsnprintf_s(str,sizeof(str),sizeof(str)-1,format, args);
	str[sizeof(str)-1] = 0;

	return CalculateTextSize(rOutputValue,ti,str,bASCIIMultiLine);
}
//////////////////////////////////////////////////////////////////////////
bool CRenderer::CalculateTextSize(Vec2& rOutputValue, SDrawTextInfo &ti,const char *text,const bool bASCIIMultiLine)
{
	// It is currently not thread-safe, but it can be called from the main 
	// thread.
	// To make it thread-safe we would need to add an Autolock here.

	// This static variable is used for lazy initialization.
	static IFFont*	pQueryFont(NULL);
	// Check for the presence of a D3D device
	if (!iSystem)
	{
		return false;
	}

	if (!m_pDefaultFont)
	{
		return false;
	}

	if (!pQueryFont)
	{
		if (!gEnv->pCryFont)
		{
			return false;
		}
		pQueryFont = gEnv->pCryFont->NewFont("renderer_internal_query");
		if (!pQueryFont->Load(RENDERER_DEFAULT_FONT) )
		{
			return false;
		}
	}
	IFFont *pFont(pQueryFont);

	pFont->SetCharWidthScale(1);

	pFont->UseFrame((ti.flags&eDrawText_Framed) != 0);

	if (ti.flags & eDrawText_Monospace)
	{
		if (ti.flags & eDrawText_FixedSize)
			pFont->SetSizeIn800x600(false);
		pFont->SetSize( vector2f(UIDRAW_TEXTSIZEFACTOR*ti.xscale,UIDRAW_TEXTSIZEFACTOR*ti.yscale) );
		pFont->SetCharWidthScale(0.5f);
		pFont->SetProportional(false);
	}
	else if (ti.flags & eDrawText_FixedSize)
	{
		pFont->SetSizeIn800x600(false);
		pFont->SetSize( vector2f(UIDRAW_TEXTSIZEFACTOR*ti.xscale,UIDRAW_TEXTSIZEFACTOR*ti.yscale) );
		pFont->SetProportional(true);
	}
	else
	{
		pFont->SetSizeIn800x600(true);
		pFont->SetProportional(false);
		pFont->SetCharWidthScale(0.5f);
		pFont->SetSize(vector2f(UIDRAW_TEXTSIZEFACTOR*ti.xscale,UIDRAW_TEXTSIZEFACTOR*ti.yscale));
	}

	rOutputValue=pFont->GetTextSize(text,bASCIIMultiLine);

	return true;
}
//////////////////////////////////////////////////////////////////////
void CRenderer::DrawLabelImage(const Vec3 &vPos,float fImageSize,int nTextureId)
{
  int nT = m_pRT->GetThreadList();

	m_TextMessages[nT].PushEntry_Texture(vPos,nTextureId,fImageSize);
}

//////////////////////////////////////////////////////////////////////
void CRenderer::FlushTextMessages()
{
	ASSERT_IS_MAIN_THREAD(m_pRT)

	if(!m_TextMessages[m_RP.m_nFillThreadID].empty())
	  m_pRT->RC_FlushTextMessages();
}

void CRenderer::RT_FlushTextMessages()
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );

  int nT = m_RP.m_nProcessThreadID;

	if (gEnv->pSystem->IsDedicated())
	{
		m_TextMessages[nT].Clear();
		return;
	}
	
	//if (GetIRenderAuxGeom())
	//	GetIRenderAuxGeom()->Flush();

  EnableFog(false);
	int vx,vy,vw,vh;
	GetViewport( &vx,&vy,&vw,&vh );

	while(const CTextMessages::CTextMessageHeader *pEntry = m_TextMessages[nT].GetNextEntry())
	{
		const CTextMessages::SText *pText = pEntry->CastTo_Text();
		const CTextMessages::STexture *pTexture = pEntry->CastTo_Texture();

		Vec3 vPos(0,0,0);
		int nDrawFlags = 0;
		const char *szText = 0;
		Vec4 vColor(1,1,1,1);
		float fSize=0;

		if(pText)
		{
			nDrawFlags = pText->m_nDrawFlags;
			szText = pText->GetText();
			vPos = pText->m_vPos;
			vColor = pText->m_Color.toVec4()*1.0f/255.0f;
			fSize = pText->m_fFontSize;
		}
		else if(pTexture)
		{
			vPos = pTexture->m_vPos;
			fSize = pTexture->m_fImageSize; 
		}

		bool b800x600 = (nDrawFlags & eDrawText_800x600) != 0;

		float fMaxPosX = 100.0f;
		float fMaxPosY = 100.0f;

		if(!b800x600)
		{
			fMaxPosX = (float)vw;
			fMaxPosY = (float)vh;
		}

    float sx,sy,sz;

		if (!(nDrawFlags & eDrawText_2D))
		{
			float fDist = 1; //GetDistance(pTextInfo->pos,GetCamera().GetPosition());
      
			float K = GetCamera().GetFarPlane()/fDist;
      if(fDist>GetCamera().GetFarPlane()*0.5)
        vPos = GetCamera().GetPosition() + K*(vPos - GetCamera().GetPosition());

			ProjectToScreen( vPos.x, vPos.y, vPos.z, &sx, &sy, &sz );
		}
		else
		{
			if (b800x600)
			{
				// Make 2D coords in range 0-100
				sx = (vPos.x) / vw * 100;
				sy = (vPos.y) / vh * 100;
			}
			else
			{
				sx = vPos.x;
				sy = vPos.y;
			}

			sz = vPos.z;
		}

    if(sx>=0 && sx<=fMaxPosX)
    if(sy>=0 && sy<=fMaxPosY)
    if(sz>=0 && sz<=1)
    {
      // calculate size
      float sizeX;
			float sizeY;
			if (nDrawFlags & eDrawText_FixedSize)
			{
				sizeX = fSize;
				sizeY = fSize;
        //sizeX = pTextInfo->font_size * 800.0f/vw;
				//sizeY = pTextInfo->font_size * 500.0f/vh;
			}
      else
			{
        sizeX = sizeY = (1.0f-(float)(sz))*32.f*fSize;
				sizeX *= 0.5f;
			}

			if (szText)
			{
	      // print
				SDrawTextInfo ti;
				ti.flags = nDrawFlags;
				ti.color[0] = vColor.x;
				ti.color[1] = vColor.y;
				ti.color[2] = vColor.z;
				ti.color[3] = vColor.w;
				ti.xscale = sizeX;
				ti.yscale = sizeY;
				if (b800x600)
					Draw2dText( 0.01f*800*sx,0.01f*600*sy,szText,ti );
				else
					Draw2dText( sx,sy,szText,ti );
			}			

			if(pTexture)
			if(pTexture->m_nTextureID>=0)
			{
        SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
        SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
				SetTexture(pTexture->m_nTextureID);				

			  Matrix44	mat;
			  GetModelViewMatrix(mat.GetData());
				Vec3 vRight( mat(0,0), mat(1,0), mat(2,0));
				Vec3 vUp( mat(0,1), mat(1,1), mat(2,1) ); 				

				DrawQuad(vRight,vUp,vPos,2);		
        SetState(GS_DEPTHWRITE);
			}
		}
	}

	m_TextMessages[nT].Clear();
}

#if !defined(PS3) && !defined(LINUX)
#pragma pack (push)
#pragma pack (1)
typedef struct
{
  unsigned char  id_length, colormap_type, image_type;
  unsigned short colormap_index, colormap_length;
  unsigned char  colormap_size;
  unsigned short x_origin, y_origin, width, height;
  unsigned char  pixel_size, attributes;
}	TargaHeader_t;
#pragma pack (pop)
#else
typedef struct
{
	unsigned char  id_length, colormap_type, image_type;
	unsigned short colormap_index _PACK, colormap_length _PACK;
	unsigned char  colormap_size;
	unsigned short x_origin _PACK, y_origin _PACK, width _PACK, height _PACK;
	unsigned char  pixel_size, attributes;
}	TargaHeader_t;
#endif

bool	CRenderer::SaveTga(unsigned char *sourcedata,int sourceformat,int w,int h,const char *filename,bool flip) const
{
  //assert(0);
//  return CImage::SaveTga(sourcedata,sourceformat,w,h,filename,flip);
	
	if (flip)
	{
		int size=w*(sourceformat/8);
		unsigned char *tempw=new unsigned char [size];
		unsigned char *src1=sourcedata;		
		unsigned char *src2=sourcedata+(w*(sourceformat/8))*(h-1);
		for (int k=0;k<h/2;k++)
		{
			memcpy(tempw,src1,size);
			memcpy(src1,src2,size);
			memcpy(src2,tempw,size);
			src1+=size;
			src2-=size;
		}
		delete [] tempw;
	}
	

	unsigned char *oldsourcedata=sourcedata;

	if (sourceformat==FORMAT_8_BIT)
	{

		unsigned char *desttemp=new unsigned char [w*h*3];
		memset(desttemp,0,w*h*3);

		unsigned char *destptr=desttemp;
		unsigned char *srcptr=sourcedata;

		unsigned char col;

		for (int k=0;k<w*h;k++)
		{			
			col=*srcptr++;
			*destptr++=col;		
			*destptr++=col;	
			*destptr++=col;		
		}
		
		sourcedata=desttemp;

		sourceformat=FORMAT_24_BIT;
	}

	TargaHeader_t header;

	memset(&header, 0, sizeof(header));
	header.image_type = 2;
	header.width = w;
	header.height = h;
	header.pixel_size = sourceformat;
  	
	unsigned char *data = new unsigned char[w*h*(sourceformat>>3)];
	unsigned char *dest = data;
	unsigned char *source = sourcedata;

	//memcpy(dest,source,w*h*(sourceformat>>3));
	
	for (int ax = 0; ax < h; ax++)
	{
		for (int by = 0; by < w; by++)
		{
			unsigned char r, g, b, a;
			r = *source; source++;
			g = *source; source++;
			b = *source; source++;
			if (sourceformat==FORMAT_32_BIT) 
			{
				a = *source; source++;
			}				
			*dest = b; dest++;
			*dest = g; dest++;
			*dest = r; dest++;
			if (sourceformat==FORMAT_32_BIT) 
			{
				*dest = a; dest++;
			}
		}
	}
	

	FILE *f = fxopen(filename,"wb");
	if (!f)
	{		
		//("Cannot save %s\n",filename);
		delete [] data;
		return (false);
	}

	if (!fwrite(&header, sizeof(header),1,f))
	{		
		//CLog::LogToFile("Cannot save %s\n",filename);
		delete [] data;
		fclose(f);
		return (false);
	}

	if (!fwrite(data, w*h*(sourceformat>>3),1,f))
	{		
		//CLog::LogToFile("Cannot save %s\n",filename);
		delete [] data;
		fclose(f);
		return (false);
	}

	fclose(f);	

	delete [] data;
	if (sourcedata!=oldsourcedata)
		delete [] sourcedata;

	return (true);	  
}

//================================================================



////////////////////////////////////////////////////////////////////////////////////////////////////

//#include "../Common/Character/CryModel.h"

void CRenderer::FreeResources(int nFlags)
{
  iLog->Log("*** Clearing render resources ***");

  if (nFlags == FRR_ALL)
    CRenderMesh2::ShutDown();

  uint32 i, j;

	if (nFlags & FRR_DELETED_MESHES)
	{
		CRenderMesh2::DeleteDelayedMeshes();
	}

  if (nFlags & FRR_SHADERS)
    gRenDev->m_cEF.ShutDown();
  if (nFlags & FRR_SYSTEM)
  {
    for (i=0; i<m_RP.m_Objects.Num(); i++)
    {
      CRenderObject *obj = m_RP.m_Objects[i];
      if (!obj)
        continue;
      if (i >= 1024)
        delete obj;
      m_RP.m_Objects[i] = NULL;
    }
    m_RP.m_Objects.Free();
    SAFE_DELETE_ARRAY(m_RP.m_ObjectsPool);
    for (j=0; j<2; j++)
    {
      for (i=0; i<m_RP.m_TempObjects[j].Num(); i++)
      {
        CRenderObject *obj = m_RP.m_TempObjects[j][i];
        if (!obj)
          continue;
        if (i >= m_RP.m_nNumObjectsInPool)
          delete obj;
        m_RP.m_TempObjects[j][i] = NULL;
      }
      m_RP.m_TempObjects[j].Free();
    }
    EF_PipelineShutdown();
  }
  if (nFlags == FRR_ALL)
    CRendElementBase::ShutDown();

  if (nFlags & FRR_TEXTURES)
    CTexture::ShutDown();

  if ((nFlags & FRR_RESTORE) && !(nFlags & FRR_SYSTEM))
    m_cEF.mfInit();
}

EScreenAspectRatio CRenderer::GetScreenAspect(int nWidth, int nHeight)
{
  EScreenAspectRatio eSA = eAspect_Unknown;

  float fNeed16_9 = 16.0f / 9.0f;
  float fNeed16_10 = 16.0f / 10.0f;
  float fNeed4_3 = 4.0f / 3.0f;

  float fCur = (float)nWidth / (float)nHeight;
  if (fabs(fCur-fNeed16_9) < 0.1f)
    eSA = eAspect_16_9;

  if (fabs(fCur-fNeed4_3) < 0.1f)
    eSA = eAspect_4_3;

  if (fabs(fCur-fNeed16_10) < 0.1f)
    eSA = eAspect_16_10;

  return eSA;
}

bool CRenderer::WriteTGA(byte *dat, int wdt, int hgt, const char *name, int src_bits_per_pixel, int dest_bits_per_pixel)
{
	return ::WriteTGA((byte*)dat, wdt, hgt, name, src_bits_per_pixel,dest_bits_per_pixel);
}

const char *sourceFile;
unsigned int sourceLine;


bool CRenderer::WriteDDS(byte *dat, int wdt, int hgt, int Size, const char *nam, ETEX_Format eFDst, int NumMips)
{
	bool bRet=true;

  byte *data = NULL;
  if (Size == 3)
  {
    data = new byte[wdt*hgt*4];
    for (int i=0;  i<wdt*hgt; i++)
    {
      data[i*4+0] = dat[i*3+0];
      data[i*4+1] = dat[i*3+1];
      data[i*4+2] = dat[i*3+2];
      data[i*4+3] = 255;
    }
    dat = data;
  }
  char name[256];
  fpStripExtension(nam, name);
  strcat(name, ".dds");

  bool bMips = false;
  if (NumMips != 1)
    bMips = true;
  int nDxtSize;
  byte *dst = CTexture::Convert(dat, wdt, hgt, NumMips, eTF_A8R8G8B8, eFDst, NumMips, nDxtSize, true);
  if (dst)
  {
    ::WriteDDS(dst, wdt, hgt, 1, name, eFDst, NumMips, eTT_2D);
    delete [] dst;
  }
  if (data)
    delete [] data;

	return bRet;
}

string *CRenderer::EF_GetShaderNames(int& nNumShaders)
{
  nNumShaders = m_cEF.m_ShaderNames.size();
  return &m_cEF.m_ShaderNames[0];
}

IShader *CRenderer::EF_LoadShader (const char *name, int flags, uint64 nMaskGen)
{
#ifdef NULL_RENDERER
  return m_cEF.m_DefaultShader;
#else
  return m_cEF.mfForName(name, flags, NULL, nMaskGen);
#endif
}

uint64 CRenderer::EF_GetRemapedShaderMaskGen( const char *name, uint64 nMaskGen, bool bFixup )
{
	return m_cEF.mfGetRemapedShaderMaskGen( name, nMaskGen, bFixup);
}

uint64 CRenderer::EF_GetShaderGlobalMaskGenFromString( const char *szShaderName, const char *szShaderGen, uint64 nMaskGen )
{
  if( !m_cEF.mfUsesGlobalFlags( szShaderName ) )
    return nMaskGen;

  return m_cEF.mfGetShaderGlobalMaskGenFromString( szShaderGen );
}

// inverse of EF_GetShaderMaskGenFromString
const char *CRenderer::EF_GetStringFromShaderGlobalMaskGen( const char *szShaderName, uint64 nMaskGen ) 
{
  if( !m_cEF.mfUsesGlobalFlags( szShaderName ) )
    return "\0";

  return m_cEF.mfGetShaderBitNamesFromGlobalMaskGen( nMaskGen );
}

SShaderItem CRenderer::EF_LoadShaderItem (const char *szName, bool bShare, int flags, SInputShaderResources *Res, uint64 nMaskGen)
{
	LOADING_TIME_PROFILE_SECTION(GetISystem());

#ifdef NULL_RENDERER
	return m_cEF.m_DefaultShaderItem;
#else
  return m_pRT->RC_LoadShaderItem(szName, bShare, flags, Res, nMaskGen);
#endif
}

//////////////////////////////////////////////////////////////////////////
bool CRenderer::EF_ReloadFile (const char *szFileName)
{
	// Replace .tif extensions with .dds extensions.
	char realName[MAX_PATH + 1];
	int nameLength = __min(strlen(szFileName), size_t(MAX_PATH));
	memcpy(realName, szFileName, nameLength);
	realName[nameLength] = 0;
	static const char* tifExtension = "tif";
	static const char* ddsExtension = "dds";
	static const int extensionLength = 3;

#if defined(_WIN32)
	if (nameLength >= extensionLength && (memcmp(realName + nameLength - extensionLength, tifExtension, extensionLength) == 0 ||
		memcmp(realName + nameLength - extensionLength, ddsExtension, extensionLength) == 0))
	{
		// Usually reloading a dds will automatically trigger the resource compiler if necessary to
		// compile the .tif into a .dds. However, this does not happen if texture streaming is
		// enabled, so we explicitly run the resource compiler here.
		memcpy(realName + nameLength - extensionLength, tifExtension, extensionLength);

		char unixNameBuffer[256];	
		fpConvertDOSToUnixName(unixNameBuffer, realName);

		char gameFolderPath[256];
		strcpy(gameFolderPath, PathUtil::GetGameFolder());
		int gameFolderPathLength = strlen(gameFolderPath);
		if (gameFolderPathLength > 0 && gameFolderPath[gameFolderPathLength - 1] == '\\')
		{
			gameFolderPath[gameFolderPathLength - 1] = '/';
		}
		else if (gameFolderPathLength > 0 && gameFolderPath[gameFolderPathLength - 1] != '/')
		{
			gameFolderPath[gameFolderPathLength++] = '/';
			gameFolderPath[gameFolderPathLength] = 0;
		}

		char* gameRelativePath = unixNameBuffer;
		if (strlen(gameRelativePath) >= (uint32)gameFolderPathLength && memcmp(gameRelativePath, gameFolderPath, gameFolderPathLength) == 0)
			gameRelativePath += gameFolderPathLength;

#ifndef XENON
		CResourceCompilerHelper().ProcessIfNeeded(gameRelativePath, true);
#endif
	}
#endif //defined(_WIN32)

	if (nameLength >= extensionLength && memcmp(realName + nameLength - extensionLength, tifExtension, extensionLength) == 0)
		memcpy(realName + nameLength - extensionLength, ddsExtension, extensionLength);

  char nmf[512];
  char drn[512];
  char drv[16];
  char dirn[512];
  char fln[128]; 
  char extn[16];
  _splitpath(realName, drv, dirn, fln, extn);
  strcpy(drn, drv);
  strcat(drn, dirn);
  strcpy(nmf, fln);
  strcat(nmf, extn);

  //TODO replace this with a single function get file type 
  //function should return and enum and the following code replaced with a switch statement
  if ( stricmp(extn,".cgf") == 0 )
  {
	  IStatObj * pStatObjectToReload = gEnv->p3DEngine->FindStatObjectByFilename(realName);
	  if ( pStatObjectToReload )
	  {
		  pStatObjectToReload->Refresh(FRO_GEOMETRY|FRO_SHADERS|FRO_TEXTURES);
	  }
  }
  else if (!stricmp(extn, ".cfx") || (!CV_r_shadersignoreincludeschanging && !stricmp(extn, ".cfi")))
  {
	  gRenDev->m_cEF.m_Bin.InvalidateCache();
	  // This is a temporary fix so that shaders would reload during hot update.
	  return gRenDev->m_cEF.mfReloadAllShaders(FRO_SHADERS, 0);
	  //    return gRenDev->m_cEF.mfReloadFile(drn, nmf, FRO_SHADERS);
  } 
  else if (!stricmp(extn, ".tif") || !stricmp(extn, ".tga") || !stricmp(extn, ".pcx") || !stricmp(extn, ".dds") || !stricmp(extn, ".jpg") || !stricmp(extn, ".gif") || !stricmp(extn, ".bmp"))
  {
	  return CTexture::ReloadFile(realName);
  }
  return false;
}

void CRenderer::EF_ReloadShaderFiles (int nCategory)
{
  //gRenDev->m_cEF.mfLoadFromFiles(nCategory);
}

void CRenderer::EF_ReloadTextures ()
{
  CTexture::ReloadTextures();
}

bool CRenderer::EF_ScanEnvironmentCM (const char *name, int size, Vec3& Pos)
{
  return CTexture::ScanEnvironmentCM(name, size, Pos);
}

bool CRenderer::EF_RenderEnvironmentCubeHDR (int size, Vec3& Pos, TArray<unsigned short>& vecData)
{
	return CTexture::RenderEnvironmentCMHDR(size, Pos, vecData);
}

int CRenderer::EF_LoadLightmap (const char *name)
{
  CTexture *tp = (CTexture *)EF_LoadTexture(name, FT_DONT_STREAM | FT_STATE_CLAMP | FT_NOMIPS, eTT_2D);
  if (tp->IsTextureLoaded())
    return tp->GetID();
  else
    return -1;
}

ITexture *CRenderer::EF_GetTextureByID(int Id)
{
  if (Id > 0)
  {
    CTexture *tp = CTexture::GetByID(Id);
    if (tp)
      return tp;
  }
  return NULL;
}

ITexture *CRenderer::EF_LoadTexture(const char* nameTex, uint32 flags, byte eTT, float fAmount1, float fAmount2)
{
#if defined (NULL_RENDERER)
  if (CTexture::s_ptexNoTexture)
    return CTexture::s_ptexNoTexture;
  else
#endif
  {
		const char *ext = fpGetExtension(nameTex);
		if(ext && stricmp(ext,".tif")==0)		// for TIF files, register by the dds file name (to not load it twice)
		{
			char name[256];
			fpStripExtension(nameTex, name);
			strcat(name, ".dds");

	    return CTexture::ForName(name, flags, eTF_Unknown, fAmount1, fAmount2);
		}
		else
      return CTexture::ForName(nameTex, flags, eTF_Unknown, fAmount1, fAmount2);
  }
}

void CRenderer::EF_StartEf ()
{
  int i, j;
	ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_RP.m_nFillThreadID;  
  if (SRendItem::m_RecurseLevel[nThreadID] <= 0)
  {
    SRendItem::m_RecurseLevel[nThreadID] = 0;
    for (j=0; j<MAX_LIST_ORDER; j++)
    {
      for (i=0; i<EFSLIST_NUM; i++)
      {
        SRendItem::RendItems(nThreadID,j,i).SetUse(0);
        SRendItem::m_AppStartRI[SRendItem::m_RecurseLevel[nThreadID]][j][i] = 0;
      }
    }
    m_RP.m_TempObjects[nThreadID].SetUse(1);
    CRenderObject::m_sObjData[nThreadID].SetUse(0);

    //SG frustums
    //SRendItem::m_ShadowGenRecurLevel[nThreadID] = 0;
    for (i=0; i<MAX_SHADOWMAP_FRUSTUMS; i++)
    {
      SRendItem::m_ShadowsStartRI[nThreadID][i] = 0;
      SRendItem::m_ShadowsEndRI[nThreadID][i] = 0;
    }

    for (i=0; i<(MAX_REND_LIGHTS+MAX_DEFERRED_LIGHTS); i++)
    {
      SRendItem::m_StartFrust[nThreadID][i] = 0;
      SRendItem::m_EndFrust[nThreadID][i] = 0;
    }

    EF_RemovePolysFromScene();
		m_RP.m_fogVolumeContibutions[nThreadID].Clear();
  }

  for (j=0; j<MAX_LIST_ORDER; j++)
  {
    for (i=0; i<EFSLIST_NUM; i++)
    {
      SRendItem::m_AppStartRI[SRendItem::m_RecurseLevel[nThreadID]][j][i] = SRendItem::RendItems(nThreadID,j,i).Num();
      SRendItem::m_BatchFlags[SRendItem::m_RecurseLevel[nThreadID]][j][i] = 0;
    }
  }
  m_RP.m_RejectedObjects[nThreadID].SetUse(0);
  //EF_PushObjectsList(nID);

	PostEffectMgr().OnBeginFrame();

  SRendItem::m_RecurseLevel[nThreadID]++;
	SRendItem::m_RenderView[nThreadID] = 0;

  EF_ClearLightsList();
}

uint32 CRenderer::EF_BatchFlags(const SShaderItem& SH, CRenderObject *pObj, int nThreadID, CRendElementBase *re)
{
  uint32 nFlags = FB_GENERAL;
  SShaderTechnique *const __restrict pTech = SH.GetTechnique();
  SRenderShaderResources *const __restrict pR = (SRenderShaderResources *)SH.m_pShaderResources;
  CShader *const __restrict pS = (CShader *)SH.m_pShader;
  if (SRendItem::m_RecurseLevel[nThreadID] == 1 && pTech)
  {
    if (!(pTech->m_Flags & FHF_POSITION_INVARIANT) && ((pTech->m_Flags & FHF_TRANSPARENT) || pObj->m_fAlpha<1.0f || (pR && pR->Opacity() < 1.0f)))
      nFlags |= FB_TRANSPARENT; 

    if ((pTech->m_nTechnique[TTYPE_Z] > 0 && (!(pObj->m_ObjFlags & FOB_NO_Z_PASS) || (pS->m_Flags2 & EF2_FORCE_ZPASS)) ))
		{
      nFlags |= FB_Z;
		
		//// Check for full-deferred rendering cases
		//if ( CV_r_deferredshading==3 &&  (pS->m_Flags & EF_SUPPORTSDEFERREDSHADING_FULL)&& (!pR || pR && !pR->GetAlphaRef())) 
		//	return nFlags;

			if( CV_r_ZPrePass && !(nFlags & FB_TRANSPARENT) && pR && !pR->GetAlphaRef() )
			{
				// Add to z-prepass if: size on screen big, nearby, no alpha test 
				if( pObj->m_fDistance < CV_r_ZPrePassDistanceThreshold && re)
				{
					// Why we don't have bbox data already pre-computed/cached ?
					Vec3 pMin, pMax;
					re->mfGetBBox(pMin, pMax);
					Matrix34A& m = pObj->GetMatrix();
					float fRadiusSq =( m.TransformPoint(pMax) - m.TransformPoint(pMin) ).len2() * 0.5f;

					if( fRadiusSq > CV_r_ZPrePassRadiusThreshold * CV_r_ZPrePassRadiusThreshold )
						nFlags |= FB_ZPREPASS;
				}
			}
			
		}

		if (pTech->m_nTechnique[TTYPE_DETAIL] > 0 && ( (pR->m_Textures[EFTT_DETAIL_OVERLAY] && !(pS->m_Flags2 & EF2_DETAILBUMPMAPPING)) || ((pR->m_ResFlags & MTL_FLAG_DETAIL_DECAL) && pR->m_Textures[EFTT_DECAL_OVERLAY] && pR->m_Textures[EFTT_CUSTOM]) ))
      nFlags |= FB_DETAIL;

    if (pTech->m_nTechnique[TTYPE_DEBUG] > 0 && 0 != (pObj->m_ObjFlags & FOB_SELECTED))
      nFlags |= FB_DEBUG;

    if (CV_r_usemateriallayers && pObj->m_nMaterialLayers && !(m_RP.m_TI[nThreadID].m_PersFlags & RBPF_SHADOWGEN))
    {
      uint32 nResourcesNoDrawFlags = pR->GetMtlLayerNoDrawFlags();

      if( (pObj->m_nMaterialLayers & MTL_LAYER_BLEND_DYNAMICFROZEN) && !(nResourcesNoDrawFlags&MTL_LAYER_FROZEN) )
        nFlags |= FB_MULTILAYERS;

      if( (pObj->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK) && !(nResourcesNoDrawFlags&MTL_LAYER_CLOAK) )
      {
        nFlags &= ~FB_TRANSPARENT; 
        nFlags |= FB_REFRACTIVE | FB_MULTILAYERS; 
      }
      else
      if ( !(nFlags & FB_MULTILAYERS) && (pObj->m_nMaterialLayers&MTL_LAYER_BLEND_WET) && pTech->m_nTechnique[TTYPE_RAINPASS] > 0 && CRenderer::CV_r_rain)
      {
        // don't apply to decals
        if( !(pObj->m_ObjFlags & FOB_DECAL) && !(pS->m_Flags & EF_DECAL) )
        {
          // Skip if beyond rain max view dist
          if( 1.0f - min(1.0f, pObj->m_fDistance / CRenderer::CV_r_rain_maxviewdist) > 0.01f )
            nFlags |= FB_RAIN;
        }
      }
    }

		if (m_bWaterCaustics && pTech->m_nTechnique[TTYPE_CAUSTICS] > 0 && (!CRenderer::CV_r_watercausticsdeferred || (nFlags & FB_TRANSPARENT)))
			nFlags |= FB_CAUSTICS;

		//if (pTech->m_nTechnique[TTYPE_SKINDIFFUSIONPASS] > 0) // && CRenderer::CV_r_watercaustics && (!CRenderer::CV_r_watercausticsdeferred || (nFlags & FB_TRANSPARENT)))
		//	nFlags |= FB_DEFERRED_SKIN_DIFFUSION;
		
    if (pTech->m_nTechnique[TTYPE_GLOWPASS] > 0 && pR->Glow())
      nFlags |= FB_GLOW;

    if (pTech->m_nTechnique[TTYPE_CUSTOMRENDERPASS] > 0 && CRenderer::CV_r_customvisions && !(pObj->m_ObjFlags & FOB_VEGETATION) && pObj->m_nVisionParams)
			nFlags |= FB_CUSTOM_RENDER;

		if (pTech->m_nTechnique[TTYPE_EFFECTLAYER] > 0 && pR->m_Textures[EFTT_CUSTOM] && pR->m_Textures[EFTT_CUSTOM_SECONDARY])
		{
			SRenderObjData *pOD = pObj->GetObjData(nThreadID);
			if ( pOD && ( (uint64)*pOD->m_pLayerEffectParams ) || CV_r_DebugLayerEffect)
				nFlags |= FB_LAYER_EFFECT;
		}

    if (pR && (pR->m_ResFlags & MTL_FLAG_SCATTER))
      nFlags |= FB_SCATTER;

    if (SH.m_nPreprocessFlags && !(m_RP.m_TI[nThreadID].m_PersFlags & RBPF_SHADOWGEN))
      nFlags |= FB_PREPROCESS;

    if (pTech->m_nTechnique[TTYPE_MOTIONBLURPASS] > 0 && pObj->m_nMotionBlurAmount )
      nFlags |= FB_MOTIONBLUR;

    // temporary solution - should rename/refactor fur pass for more general usage
    //if ((CV_r_fur || CV_r_hair_sorting_quality!=0) && pTech->m_nTechnique[TTYPE_FURPASS] > 0 && pR->FurAmount() )
    if (CV_r_fur && pTech->m_nTechnique[TTYPE_FURPASS] > 0 && pR->FurAmount() )
      nFlags |= FB_FUR;

    if (CV_r_refraction && (pS->m_Flags & EF_REFRACTIVE))
      nFlags |= FB_REFRACTIVE;

    //if (CV_r_usewaterparticles && pS && (pS->m_Flags & EF_WATERPARTICLE))
    //  nFlags = FB_WATERPARTICLES;
  }
  else
  if( SRendItem::m_RecurseLevel[nThreadID] > 1 && pTech && m_RP.m_TI[nThreadID].m_PersFlags2&RBPF_MIRRORCAMERA)
  {
    if (!(pTech->m_Flags & FHF_POSITION_INVARIANT) && ((pTech->m_Flags & FHF_TRANSPARENT) || pObj->m_fAlpha<1.0f || (pR && pR->Opacity() < 1.0f)))
      nFlags |= FB_TRANSPARENT;
    if (pTech->m_nTechnique[TTYPE_CAUSTICS] > 0)
      nFlags |= FB_CAUSTICS;    
  }

  return nFlags;
}

#ifdef _DEBUG
static float sMatIdent[12] = 
{
  1,0,0,0,
  0,1,0,0,
  0,0,1,0
};
#endif

void CRenderer::EF_AddEf_NotVirtual (CRendElementBase *re, const SShaderItem& SH, CRenderObject *obj, int nList, int nAW)
{
  assert(nList>0 && nList<EFSLIST_NUM);  
  
  if (!re || !SH.m_pShader)
    return;
  
  CShader *const __restrict pSH = (CShader *)SH.m_pShader;
  if ( pSH->m_Flags2 & EF2_NODRAW )
    return;

  ASSERT_IS_MAIN_THREAD(m_pRT)

  // For nearest geometry (weapons/arms) - make sure its rendered really at beginning (before water passes)
  if( obj->m_ObjFlags & FOB_NEAREST )
    nAW = 0;

  int nThreadID = m_RP.m_nFillThreadID;
  SThreadInfo& RESTRICT_REFERENCE rTI = m_RP.m_TI[nThreadID];

  if (pSH->m_Flags & EF_RELOADED)
  {
    pSH->m_Flags &= ~EF_RELOADED;
    SShaderItem SHCopy = SH;
    SHCopy.PostLoad();
  }

	if (rTI.m_PersFlags & RBPF_SHADOWGEN)
	{
		if (!pSH->m_HWTechniques.Num() || pSH->m_HWTechniques[0]->m_nTechnique[TTYPE_SHADOWGEN] < 0)
			return;
	}

#ifdef _DEBUG
  if (memcmp(sMatIdent, obj->m_II.m_Matrix.GetData(), 3*4*4))
  {
    if (!(obj->m_ObjFlags & FOB_TRANS_MASK))
    {
      assert(0);
    }
  }
#endif







  
  uint32 nBatchFlags = EF_BatchFlags(SH, obj, nThreadID, re);
  //assert(!(nBatchFlags & FB_SCATTER) || nList == EFSLIST_GENERAL);

	if ( obj->m_ObjFlags & FOB_RENDER_AFTER_POSTPROCESSING && CRenderer::CV_r_PostProcess && CRenderer::CV_r_PostProcessHUD3D )
	{
		CPostEffect *pPostEffect = PostEffectMgr().GetEffect(ePFX_3DHUD);
		pPostEffect->AddRE( re, &SH, obj );
		return;
	}

  if ((FOB_ONLY_Z_PASS & obj->m_ObjFlags) || CV_r_ZPassOnly)
  {
    if (!(nBatchFlags & (FB_TRANSPARENT | FB_SCATTER)))
      nBatchFlags = FB_Z;
  }
	

	if ( nBatchFlags & FB_PREPROCESS ) 
	{
		EShaderType pSHType = pSH->GetShaderType();

		// Prevent water usage on non-water specific meshes (it causes reflections updates). Todo: this should be checked in editor side and not allow such usage
		if( pSHType != eST_Water || pSHType == eST_Water && nList == EFSLIST_WATER ) 
			SRendItem::mfAdd(nThreadID, re, obj, SH, EFSLIST_PREPROCESS, 0, nBatchFlags);
	}

  if (nBatchFlags & FB_REFRACTIVE)
  {
    nBatchFlags &= ~FB_Z;
    nList = EFSLIST_REFRACTPASS;
  }
  else
  if (nBatchFlags & FB_TRANSPARENT)
  {
    if (nList == EFSLIST_GENERAL)
      nList = EFSLIST_TRANSP;
  }
  //else
  //if (nBatchFlags & FB_SCATTER)
  //  nBatchFlags &= ~(FB_GENERAL | FB_Z);

  // make sure decals go into proper render list 
  // also, set additional shadow flag (i.e. reuse the shadow mask generated for underlying geometry)
  // TODO: specify correct render list and additional flags directly in the engine once non-material decal rendering support is removed! 
  if (((obj->m_ObjFlags & FOB_DECAL) || (pSH->m_Flags & EF_DECAL)))
  {         
    static ICVar *pVar = iConsole->GetCVar("e_Decals");
    if( pVar && pVar->GetIVal() == 0)
      return;

    if (nList != EFSLIST_GENERAL && nList != EFSLIST_TERRAINLAYER) 
      nBatchFlags &= ~FB_Z;

    SShaderTechnique *pTech = SH.GetTechnique();    
    if (pTech && pTech->m_nTechnique[TTYPE_Z] > 0 && ((pSH->m_Flags2 & EF2_FORCE_ZPASS) || CV_r_deferredshading))
    {
      // temporary workaround: if no normal map use base normal from NormalsTarget          
      // todo: make this instead by decal type
      if ((pSH->m_Flags2 & EF2_FORCE_ZPASS) || SH.m_pShaderResources && SH.m_pShaderResources->GetTexture(EFTT_BUMP) || CV_r_deferredshading ==3 )         
        nBatchFlags |= FB_Z; 
    }

    nList = EFSLIST_DECAL;
    obj->m_ObjFlags |= FOB_INSHADOW;

		if ((obj->m_ObjFlags & FOB_DECAL) == 0 && SH.m_pShaderResources)
			obj->m_nSort = ((SRenderShaderResources*) SH.m_pShaderResources)->m_SortPrio;
  }
  else
  if (nList != EFSLIST_GENERAL && nList != EFSLIST_TERRAINLAYER) 
    nBatchFlags &= ~FB_Z;

  const uint32 nForceFlags = (EF2_FORCE_DRAWLAST|EF2_FORCE_DRAWFIRST|EF2_FORCE_ZPASS|EF2_FORCE_TRANSPASS|EF2_FORCE_GENERALPASS|EF2_FORCE_DRAWAFTERWATER|EF2_FORCE_WATERPASS|EF2_AFTERHDRPOSTPROCESS);
  if (pSH->m_Flags2 & nForceFlags )
  {
    // Force rendering in last place
    if (pSH->m_Flags2 & EF2_FORCE_DRAWLAST)
      obj->m_fSort -= 100000.0f;
    if (pSH->m_Flags2 & EF2_FORCE_DRAWFIRST)
      obj->m_fSort += 100000.0f;

    if( pSH->m_Flags2 & EF2_FORCE_ZPASS && !( (nBatchFlags & FB_REFRACTIVE) && (nBatchFlags &FB_MULTILAYERS) ) )  
      nBatchFlags |= FB_Z;

    if( pSH->m_Flags2 & EF2_FORCE_TRANSPASS ) 
      nList = EFSLIST_TRANSP;
    else
    if( pSH->m_Flags2 & EF2_FORCE_GENERALPASS )
      nList = EFSLIST_GENERAL;
    else
    if( pSH->m_Flags2 & EF2_FORCE_WATERPASS )
      nList = EFSLIST_WATER;
    if( pSH->m_Flags2 & EF2_AFTERHDRPOSTPROCESS ) 
		{
			if ( pSH->m_Flags2 & EF2_FORCE_DRAWLAST )
				nList = EFSLIST_AFTER_POSTPROCESS;
			else
				nList = EFSLIST_AFTER_HDRPOSTPROCESS;
		}

    if( pSH->m_Flags2& EF2_FORCE_DRAWAFTERWATER )
      nAW = 1;
  }

  //if (nBatchFlags & FB_WATERPARTICLES)
  //{
  //  nBatchFlags = FB_TRANSPARENT;
  //  nList = EFSLIST_WATERPARTICLES;
  //}

  if (rTI.m_PersFlags & RBPF_SHADOWGEN)
  {
    SRendItem::mfAdd(nThreadID, re, obj, SH, EFSLIST_SHADOW_GEN, SG_SORT_GROUP, nBatchFlags);
  }
  else
  {
    // Always force cloaked geometry to render after water
    if (obj->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK)
      nAW = 1;

		// Temporary way to enforce sky and sprites to render last, since:
		// 1. we sort by light mask for zpass atm
		// 2. we don't have any shader information during resources sorting (mfSortResources) which takes higher priority than shader ID
		if ((pSH->m_Flags & EF_SKY) || (pSH->m_Flags2 & EF2_PREPR_GENSPRITES))
			obj->m_DynLMMask[nThreadID] = ~0;
 
    SRendItem::mfAdd(nThreadID, re, obj, SH, nList, nAW, nBatchFlags);
  }
}

void CRenderer::RT_FlashRender(IFlashPlayer_RenderProxy* pPlayer, bool stereo)
{
	m_pRT->RC_FlashRender(pPlayer, stereo);
}

void CRenderer::RT_FlashRemoveTexture(ITexture* pTexture)
{
	m_pRT->RC_FlashRemoveTexture((CTexture*) pTexture);
}

void CRenderer::RT_DrawStringW(IFFont *pFont, float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine)
{
  m_pRT->RC_DrawStringW(pFont, fBaseX, fBaseY, fBaseZ, szMsg, bASCIIMultiLine);
}


void CRenderer::DrawStringW(IFFont *pFont, float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine)
{
  m_pRT->RC_DrawStringW(pFont, fBaseX, fBaseY, fBaseZ, szMsg, bASCIIMultiLine);
}


void CRenderer::EF_AddEf (CRendElementBase *re, SShaderItem& pSH, CRenderObject *obj, int nList, int nAW)
{
  EF_AddEf_NotVirtual (re, pSH, obj, nList, nAW);
}

void CRenderer::RT_CreateREPostProcess(CRendElementBase **re)
{
  *re = new CREPostProcess;
}

CRendElementBase *CRenderer::EF_CreateRE( EDataType edt )
{
  CRendElementBase *re = NULL;
  switch(edt)
  {
    case eDATA_Mesh:
      re = new CREMesh;
      break;

    case eDATA_Imposter:
      re = new CREImposter;
      break;

		case eDATA_PanoramaCluster:
			re = new CREPanoramaCluster;
			break;

    case eDATA_HDRProcess:
      re = new CREHDRProcess;
      break;

    case eDATA_DeferredShading:
      re = new CREDeferredShading;
      break;

    case eDATA_OcclusionQuery:
      re = new CREOcclusionQuery;
      break;

    case eDATA_Ocean:
      assert(0);
      break;

    case eDATA_Flare:
      re = new CREFlare;
      break;

    case eDATA_Cloud:
      re = new CRECloud;
      break;

    case eDATA_Sky:
      re = new CRESky;
      break;

		case eDATA_HDRSky:
			re = new CREHDRSky;
			break;

    case eDATA_Beam:
      re = new CREBeam;
      break;

    case eDATA_TerrainSector:
      re = new CRECommon;
      break;

    case eDATA_FarTreeSprites:
      re = new CREFarTreeSprites;
      break;

    case eDATA_Dummy:
      re = new CREDummy;
      break;

    case eDATA_PostProcess:
			m_pRT->RC_CreateREPostProcess(&re);
			m_pREPostProcess=static_cast<CREPostProcess*>(re);
      break;
    
    case eDATA_ShadowMapGen:
      re = new CREShadowMapGen;
      break;
	
    case eDATA_TerrainDetailTextureLayers:
      re = new CRETerrainDetailTextureLayers;
      break;

    case eDATA_TerrainParticles:
      re = new CRETerrainParticles;
      break;
      
		case eDATA_FogVolume:
			re = new CREFogVolume;
			break;

		case eDATA_WaterVolume:
			re = new CREWaterVolume;
			break;

    case eDATA_WaterWave:
      re = new CREWaterWave;
      break;

    case eDATA_WaterOcean:
      re = new CREWaterOcean;
      break;

		case eDATA_VolumeObject:
			re = new CREVolumeObject;
			break;

		case eDATA_IrradianceVolume:
			re = new CREIrradianceVolume;
			break;

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
		case eDATA_PrismObject:
			re = new CREPrismObject;
			break;
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

		case eDATA_GameEffect:
			re = new CREGameEffect;
			break;
  }
  return re;
}

void CRenderObject::Init(int nThreadID)
{
  m_ObjFlags &= FOB_PERSISTENT;
  if (!(m_ObjFlags & FOB_PERMANENT))
    m_nObjDataId = -1;
  m_nRenderQuality = 65535;
  m_DynLMMask[nThreadID] = 0;
  m_RState = 0;
  m_fDistance = 0.0f;

  m_nMotionBlurAmount = 0;
  m_nMaterialLayers = 0;
  m_DissolveRef = 0;

  m_nMDV = 0;
  m_fSort = 0;
  m_pBending = NULL;
  m_pShadowCasters = NULL;

  m_II.m_AmbColor = Col_White;
  m_fAlpha = 1.0f;
  m_nTextureID = -1;
  m_nCBID = -1;
  m_pCurrMaterial = NULL;

  m_nVisionParams = 0;

  m_pRenderNode = NULL;
}


CRenderObject::~CRenderObject()
{
  /*if (m_ShaderParams && m_bShaderParamCreatedInRenderer)
  {
    m_bShaderParamCreatedInRenderer = false;
    delete m_ShaderParams;
  }*/
}

TArray<SRenderObjData> CRenderObject::m_sObjData[2];
SRenderObjData *CRenderObject::m_pPermObjData;
int CRenderObject::m_nPermObjDataID;
TArray<int> CRenderObject::m_sFreePermObjData;

void CRenderObject::Tick()
{
  CRenderer *rd = gRenDev;
	ASSERT_IS_MAIN_THREAD(rd->m_pRT)
  int nFrame = rd->m_nFrameSwapID - 3;
  TArray<CRenderObject *>& Objs = rd->m_RP.m_FreeObjects[nFrame & 3];
  int nThreadID = rd->m_RP.m_nFillThreadID;
  for (uint32 i=0; i<Objs.Num(); i++)
  {
    CRenderObject *pObj = Objs[i];
    pObj->m_ObjFlags |= FOB_REMOVED;
    if (pObj->m_nObjDataId>=0)
    {
      m_sFreePermObjData.push_back(pObj->m_nObjDataId);
      SRenderObjData *pOD = &m_pPermObjData[pObj->m_nObjDataId];
      if (pOD->m_pRE && pOD->m_pRE->mfGetType() == eDATA_OcclusionQuery)
      {
        pOD->m_pRE->Release(true);
        pOD->m_pRE = NULL;
      }
      pObj->m_nObjDataId = -1;
    }
#if defined (DIRECT3D10) || defined(PS3)
    if (pObj->m_nCBID >= 0)
    {
      rd->m_pRT->RC_ReleaseCB_SI(pObj->m_nCBID);
      pObj->m_nCBID = -1;
    }
#endif
    assert(rd->m_RP.m_Objects[pObj->m_Id] == pObj);
  }
  Objs.SetUse(0);
}

void CRenderer::EF_ObjRemovePermanent(CRenderObject *pObj)
{
 // CRenderObject *pObjStorage = m_RP.m_Objects[pObj->m_Id]; //Diesel cut
 // assert(pObj == pObjStorage); //Diesel cut
 // if (pObj != pObjStorage) //Diesel cut
 //   return; //Diesel cut
  int nFrame = m_nFrameSwapID;
  m_RP.m_FreeObjects[nFrame & 3].AddElem(pObj); 
}

void SRenderObjData::Init()
{
  m_Constants.Free();
  m_pLMTCBufferO = NULL;
  //m_pInstancingInfo = NULL;
  m_pRE = NULL;
  m_pVisArea = NULL;
  m_HMAData = 0;
  m_pLightImage = NULL;
  m_nFrameLight = 0;
  m_nLightStyle = 0;
  m_pWeights = NULL;
  m_pCharInstance = NULL;
  m_scissorX = m_scissorY = m_scissorWidth = m_scissorHeight = 0;
	m_pLayerEffectParams[0] = m_pLayerEffectParams[1] = 0;
}

SRenderObjData *CRenderer::EF_GetObjData(CRenderObject *pObj, bool bCreate)
{
	ASSERT_IS_MAIN_THREAD(m_pRT)
  if (pObj->m_nObjDataId<0)
  {
    if (!bCreate)
      return NULL;
    if (pObj->m_ObjFlags & FOB_PERMANENT)
    {
      int n, nID;
      if (n=CRenderObject::m_sFreePermObjData.size())
      {
        nID = CRenderObject::m_sFreePermObjData[n-1];
        CRenderObject::m_sFreePermObjData.pop_back();
      }
      else
        nID = CRenderObject::m_nPermObjDataID++;
      assert(nID < MAX_PERM_OBJECTS && nID>=0);
      nID &= (MAX_PERM_OBJECTS-1);
      pObj->m_nObjDataId = nID;
      CRenderObject::m_pPermObjData[nID].Init();
      return &CRenderObject::m_pPermObjData[nID];
    }
    else
    {
      int nThreadID = m_RP.m_nFillThreadID;
      pObj->m_nObjDataId = CRenderObject::m_sObjData[nThreadID].size();
      SRenderObjData *pD = CRenderObject::m_sObjData[nThreadID].AddIndex(1);
      memset(pD, 0, sizeof(SRenderObjData));
      pD->m_FogVolumeContribIdx[nThreadID] = -1;
      return pD;
    }
  }
  return pObj->GetObjData(m_RP.m_nFillThreadID);
}
SRenderObjData *CRenderer::FX_GetObjData(CRenderObject *pObj, bool bCreate)
{
  assert(m_pRT->IsRenderThread());
  if (pObj->m_nObjDataId<0)
  {
    if (!bCreate)
      return NULL;
    if (pObj->m_ObjFlags & FOB_PERMANENT)
    {
      int n, nID;
      if (n=CRenderObject::m_sFreePermObjData.size())
      {
        nID = CRenderObject::m_sFreePermObjData[n-1];
        CRenderObject::m_sFreePermObjData.pop_back();
      }
      else
        nID = CRenderObject::m_nPermObjDataID++;
      assert(nID < MAX_PERM_OBJECTS && nID>=0);
      nID &= (MAX_PERM_OBJECTS-1);
      pObj->m_nObjDataId = nID;
      CRenderObject::m_pPermObjData[nID].Init();
      CRenderObject::m_pPermObjData[nID].m_FogVolumeContribIdx[m_RP.m_nProcessThreadID] = -1;
      return &CRenderObject::m_pPermObjData[nID];
    }
    else
    {
      int nThreadID = m_RP.m_nProcessThreadID;
      pObj->m_nObjDataId = CRenderObject::m_sObjData[nThreadID].size();
      SRenderObjData *pD = CRenderObject::m_sObjData[nThreadID].AddIndex(1);
      memset(pD, 0, sizeof(SRenderObjData));
      pD->m_FogVolumeContribIdx[nThreadID] = -1;
      return pD;
    }
  }
  return pObj->GetObjData(m_RP.m_nProcessThreadID);
}

CREMesh *CRenderer::EF_GetTempMeshRE()
{
  CREMesh **pREs = m_RP.m_TempREs.AddIndex(1);
  CREMesh *pRE = *pREs;
  if (!pRE)
  {
    pRE = new CREMesh;
    *pREs = pRE;
  }
  return pRE;
}

void CRenderObject::CopyTo(CRenderObject *pObjNew)
{
	ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
  int n0 = pObjNew->m_Id;
  *pObjNew = *this;
  pObjNew->m_Id = n0;
  if (!(m_ObjFlags & FOB_PERMANENT))
    pObjNew->m_nObjDataId = m_nObjDataId;
  else
  {
    pObjNew->m_nObjDataId = -1;
    pObjNew->m_ObjFlags &= ~(FOB_PERMANENT | FOB_REMOVED);
    SRenderObjData *pOD = GetObjData(gRenDev->m_RP.m_nFillThreadID);
    if (pOD)
    {
      SRenderObjData *pODNew = gRenDev->EF_GetObjData(pObjNew, true);
      *pODNew = *pOD;
    }
  }
}

CRenderObject *CRenderer::EF_GetObject(bool bTemp, int num, bool bInit)
{
  CRenderObject *obj;
  uint32 i;
  
  ASSERT_IS_MAIN_THREAD(m_pRT)

  int nThreadID = m_RP.m_nFillThreadID;
  if (num >= 0)
    obj = m_RP.m_Objects[num];
  else
  {
    TArray <CRenderObject *>& Objs = bTemp ? m_RP.m_TempObjects[nThreadID] : m_RP.m_Objects;
  	uint32 n = Objs.Num();
    if (bTemp && (i=m_RP.m_RejectedObjects[nThreadID].Num()))
    {
      i--; 
      obj = m_RP.m_RejectedObjects[nThreadID][i];
      m_RP.m_RejectedObjects[nThreadID].SetUse(i);
      obj->Init(nThreadID);
      assert(!(obj->m_ObjFlags & FOB_PERMANENT));
      return obj;
    }
    if (!bTemp)
    {
      for (i=1; i<Objs.Num(); i++)
      {
        if (!Objs[i] || (Objs[i]->m_ObjFlags & FOB_REMOVED))
          break;
      }
      if (i != Objs.Num())
      {
        n = i;
        if (Objs[i])
          Objs[i]->m_ObjFlags &= ~FOB_REMOVED;
      }
      else
		    Objs.GrowReset(1);
    }
    else
		  Objs.GrowReset(1);
		obj = Objs[n];

    if (!obj)
    {
      obj = new CRenderObject;
      Objs[n] = obj;
      if (!bTemp)
        obj->m_ObjFlags |= FOB_PERMANENT;
    }
    assert(!bTemp || !(obj->m_ObjFlags & FOB_PERMANENT));





    obj->m_Id = n;
    if (bInit)
      obj->Init(nThreadID);
  } 

  SRenderObjData* pOD = obj->GetObjData(nThreadID);
  if (pOD)
    pOD->m_FogVolumeContribIdx[nThreadID] = (uint16) -1;

  return obj;
}

float CRenderer::EF_GetWaterZElevation(float fX, float fY)
{
  I3DEngine *eng = (I3DEngine *)gEnv->p3DEngine;
  if (!eng)
    return 0;
  return eng->GetWaterLevel();
}

void CRenderer::EF_RemovePolysFromScene()
{  
  ASSERT_IS_MAIN_THREAD(m_pRT)

  for (int i=0; i<MAX_RECURSION_LEVELS; i++)
  {
    CREClientPoly::m_PolysStorage[m_RP.m_nFillThreadID][i].SetUse(0);
  }
  m_RP.m_SysVertexPool[m_RP.m_nFillThreadID].SetUse(0);
  m_RP.m_SysIndexPool[m_RP.m_nFillThreadID].SetUse(0);
  m_RP.m_Sprites.SetUse(0);
  m_RP.m_Polygons.SetUse(0);
}

CRenderObject *CRenderer::MergePolygonRO(CRenderObject* pRO)
{
	uint32 i;

	if (pRO)
	{
		SRefSprite *rs = NULL;
		for (i=0; i<m_RP.m_Polygons.Num(); i++)
		{
			rs = &m_RP.m_Polygons[i];
			if (pRO == rs->m_pObj)
				break;
		}
		if (i == m_RP.m_Polygons.Num())
		{
			for (i=0; i<m_RP.m_Polygons.Num(); i++)
			{
				rs = &m_RP.m_Polygons[i];
				if (rs->m_pObj->m_DynLMMask[m_RP.m_nFillThreadID] == pRO->m_DynLMMask[m_RP.m_nFillThreadID] && rs->m_pObj->m_RState == pRO->m_RState && rs->m_pObj->m_fSort == pRO->m_fSort &&
					rs->m_pObj->m_II.m_AmbColor == pRO->m_II.m_AmbColor && rs->m_pObj->m_fAlpha == pRO->m_fAlpha)
				{
					m_RP.m_RejectedObjects[m_RP.m_nFillThreadID].AddElem(pRO);
					pRO = rs->m_pObj;
					break;
				}
			}
			if (i == m_RP.m_Polygons.Num())
			{
				SRefSprite s;
				s.m_pObj = pRO;
				//obj->m_Matrix.SetIdentity();
				m_RP.m_Polygons.AddElem(s);
				rs = &m_RP.m_Polygons[m_RP.m_Polygons.Num()-1];
			}
		}
	} 

	return pRO;
}

CRenderObject *CRenderer::EF_AddPolygonToScene(const SShaderItem& si, int numPts, const SVF_P3F_C4B_T2F *verts, const SPipTangents *tangs, CRenderObject *obj, uint16 *inds, int ninds, int nAW, bool bMerge)
{
  ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_RP.m_nFillThreadID;
	const uint32 nPersFlags = m_RP.m_TI[nThreadID].m_PersFlags;
	if (bMerge)
		obj = MergePolygonRO(obj);
  int nR = SRendItem::m_RecurseLevel[nThreadID]-1;
	
  int num = CREClientPoly::m_PolysStorage[nThreadID][nR].Num();
  CREClientPoly::m_PolysStorage[nThreadID][nR].GrowReset(1);

  CREClientPoly *pl = CREClientPoly::m_PolysStorage[nThreadID][nR][num];
  if (!pl)
  {
    pl = new CREClientPoly;
    CREClientPoly::m_PolysStorage[nThreadID][nR][num] = pl;
  }

  pl->m_Shader = si;
  pl->m_sNumVerts = numPts;
  pl->m_pObject = obj;
	pl->m_nCPFlags = 0;
	if(nAW)
		pl->m_nCPFlags |= CREClientPoly::efAfterWater;
	if(nPersFlags & RBPF_SHADOWGEN)
		pl->m_nCPFlags |= CREClientPoly::efShadowGen;

  int nSize = CRenderMesh2::m_cSizeVF[eVF_P3F_C4B_T2F] * numPts;
  int nOffs = m_RP.m_SysVertexPool[nThreadID].Num();
  SVF_P3F_C4B_T2F *vt = (SVF_P3F_C4B_T2F *)m_RP.m_SysVertexPool[nThreadID].GrowReset(nSize);
  pl->m_nOffsVert = nOffs;
  for (int i=0; i<numPts; i++, vt++)
  {
    vt->xyz = verts[i].xyz;
    vt->st = verts[i].st;
    vt->color.dcolor = verts[i].color.dcolor;
  }
  if (tangs)
  {
    nSize = sizeof(SPipTangents) * numPts;
    nOffs = m_RP.m_SysVertexPool[nThreadID].Num();
    SPipTangents *t = (SPipTangents *)m_RP.m_SysVertexPool[nThreadID].GrowReset(nSize);
    pl->m_nOffsTang = nOffs;
    for (int i=0; i<numPts; i++, t++)
    {
      *t = tangs[i];
    }
  }
  else
    pl->m_nOffsTang = -1;


  pl->m_nOffsInd = m_RP.m_SysIndexPool[nThreadID].Num();

  if (inds && ninds)
  {
    uint16* dstind = m_RP.m_SysIndexPool[nThreadID].GrowReset(ninds);
    memcpy(dstind, inds, ninds*sizeof(uint16));
    pl->m_sNumIndices = ninds;
  }
  else
  {
    uint16* dstind = m_RP.m_SysIndexPool[nThreadID].GrowReset((numPts-2) * 3);
    for (int i=0; i<numPts-2; i++, dstind+=3)
    {
      dstind[0] = 0;
      dstind[1] = i+1;
      dstind[2] = i+2;
    }
    pl->m_sNumIndices = (numPts-2) * 3;
  }

  return obj;
}

CRenderObject* CRenderer::EF_AddPolygonToScene(const SShaderItem& si, CRenderObject* obj, int numPts, int ninds, SVF_P3F_C4B_T2F*& verts, SPipTangents*& tangs, uint16*& inds, int nAW, bool bMerge)
{
  ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_RP.m_nFillThreadID;
	const uint32 nPersFlags = m_RP.m_TI[nThreadID].m_PersFlags;
	if (bMerge)
		obj = MergePolygonRO(obj);
  int nR = SRendItem::m_RecurseLevel[nThreadID]-1;

	int num = CREClientPoly::m_PolysStorage[nThreadID][nR].Num();
	CREClientPoly::m_PolysStorage[nThreadID][nR].GrowReset(1);

	CREClientPoly *pl = CREClientPoly::m_PolysStorage[nThreadID][nR][num];
	if (!pl)
	{
		pl = new CREClientPoly;
		CREClientPoly::m_PolysStorage[nThreadID][nR][num] = pl;
	}

	pl->m_Shader = si;
	pl->m_pObject = obj;
	if(nAW)
		pl->m_nCPFlags |= CREClientPoly::efAfterWater;
	if(nPersFlags & RBPF_SHADOWGEN)
		pl->m_nCPFlags |= CREClientPoly::efShadowGen;

	//////////////////////////////////////////////////////////////////////////
	// allocate buffer space for caller to fill

	pl->m_sNumVerts = numPts;
	pl->m_nOffsVert = m_RP.m_SysVertexPool[nThreadID].Num();
	pl->m_nOffsTang = m_RP.m_SysVertexPool[nThreadID].Num() + sizeof(SVF_P3F_C4B_T2F) * numPts;
	m_RP.m_SysVertexPool[nThreadID].GrowReset( (sizeof(SVF_P3F_C4B_T2F) + sizeof(SPipTangents)) * numPts );
	verts = (SVF_P3F_C4B_T2F*) &m_RP.m_SysVertexPool[nThreadID][pl->m_nOffsVert];
	tangs = (SPipTangents*) &m_RP.m_SysVertexPool[nThreadID][pl->m_nOffsTang];

	pl->m_sNumIndices = ninds;
	pl->m_nOffsInd = m_RP.m_SysIndexPool[nThreadID].Num();
	inds = (uint16*) m_RP.m_SysIndexPool[nThreadID].GrowReset( ninds );

	//////////////////////////////////////////////////////////////////////////

	return obj;
}

//================================================================================================================

CRenderObject* CRenderer::MergeParticleRO(CRenderObject* pRO)
{
	uint32 i;

	if (pRO)
	{
		FUNCTION_PROFILER_SYS(PARTICLE);

		SRefSprite *rs = NULL;
		for (i=0; i<m_RP.m_Sprites.Num(); i++)
		{
			rs = &m_RP.m_Sprites[i];
			if (pRO == rs->m_pObj)
				break;
		}
		if (i == m_RP.m_Sprites.Num())
		{
			for (i=0; i<m_RP.m_Sprites.Num(); i++)
			{
				rs = &m_RP.m_Sprites[i];
				if (rs->m_pObj->m_nTextureID == pRO->m_nTextureID && rs->m_pObj->m_DynLMMask[m_RP.m_nFillThreadID] == pRO->m_DynLMMask[m_RP.m_nFillThreadID] && rs->m_pObj->m_RState == pRO->m_RState && rs->m_pObj->m_fSort == pRO->m_fSort &&
					( !((rs->m_pObj->m_ObjFlags ^ pRO->m_ObjFlags) & FOB_PARTICLE_MASK) ) &&
					    /*rs->m_pObj->m_FogVolumeContribIdx == pRO->m_FogVolumeContribIdx && */rs->m_pObj->m_II.m_AmbColor == pRO->m_II.m_AmbColor)
				{
					m_RP.m_RejectedObjects[m_RP.m_nFillThreadID].AddElem(pRO);
					pRO = rs->m_pObj;
					break;
				}
			}
			if (i == m_RP.m_Sprites.Num())
			{
				SRefSprite s;
				s.m_pObj = pRO;
				//obj->m_Matrix.SetIdentity();
				m_RP.m_Sprites.AddElem(s);
				rs = &m_RP.m_Sprites[m_RP.m_Sprites.Num()-1];
			}
		}
	}

	return pRO;
}

//////////////////////////////////////////////////////////////////////////
#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
void CRenderer::EF_AddGPUParticlesToScene( int32 nGPUParticleIdx, AABB const& bb, const SShaderItem& shaderItem, CRenderObject* pRO, bool nAW, bool canUseGS )
{
	//pRO = MergeParticleRO( pRO );

	CREParticleGPU* pParticle = CREParticleGPU::Create( nGPUParticleIdx, bb );	
	int afterWater = 0;

	if( nAW == true )
	{
		afterWater = 1;
	}
	else if( nAW == false )
	{
		afterWater = 0;	
	}

	if( pRO != NULL )
	{
		//if( shaderItem.m_nPreprocessFlags != 0 )
		//{
		//		SRendItem::mfAdd( pParticle, pRO, shaderItem, EFSLIST_PREPROCESS, afterWater );
		//	}

		SRendItem::mfAdd (m_pRT->GetThreadList(), pParticle, pRO, shaderItem, EFSLIST_TRANSP, afterWater, FB_GENERAL);

		//	SShaderTechnique *pTech = shaderItem.GetTechnique();
		//	if( CV_r_glow && pTech && pTech->m_nTechnique[TTYPE_GLOWPASS] > 0 && shaderItem.m_pShaderResources && shaderItem.m_pShaderResources->m_GlowAmount )
		//	{
		//		SRendItem::mfAdd(pParticle, pRO, shaderItem, EFSLIST_GLOWPASS, afterWater ); 
		//	}    
	}

}
#endif

CREParticle* CRenderer::EF_AddParticlesToScene(const SShaderItem& si, CRenderObject* pRO, IParticleVertexCreator* pPVC, int nAW, bool& canUseGS)
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );
  ASSERT_IS_MAIN_THREAD(m_pRT)

	// Merge/sorting interaction currently broken.
	// pRO = MergeParticleRO(pRO);

	SShaderItem shaderItem(si);

#if defined(DIRECT3D10)
	if (!CV_r_UseGSParticles)
		canUseGS = false;
	SShaderTechnique* pTech = shaderItem.GetTechnique();
	if (pTech && (pTech->m_Flags & FHF_USE_GEOMETRY_SHADER) != 0)
	{
		if (!canUseGS)
			shaderItem.m_nTechnique = shaderItem.m_nTechnique >= 0 ? shaderItem.m_nTechnique + 1 : 1;
	}
	else
		canUseGS = false;
#else
		canUseGS = false;
#endif

	SParticleRenderContext context;
	context.m_vCamPos = GetCamera().GetPosition();
	context.m_vCamDir = GetCamera().GetViewdir();
	context.m_fAngularRes = GetHeight() / GetCamera().GetFov();

	CREParticle* pParticle = CREParticle::Create(pPVC, context);
	EF_AddParticle(pParticle, shaderItem, pRO, nAW);
	return pParticle;
}

void CRenderer::EF_AddParticle(CREParticle* pParticle, const SShaderItem& shaderItem, CRenderObject* pRO, int nAW)
{
  if (pRO)
  {
    uint32 nBatchFlags = FB_GENERAL;
    int nList = EFSLIST_TRANSP;
    if ( shaderItem.m_pShader && (((CShader *)shaderItem.m_pShader)->m_Flags & EF_REFRACTIVE) )
    {
      if( CV_r_refraction && CV_r_useparticles_refraction )
      {
        nBatchFlags |= FB_REFRACTIVE;
        nList = EFSLIST_REFRACTPASS;
      }
      else 
        return;  // skip adding refractive particle
    }

    SShaderTechnique* pTech(shaderItem.GetTechnique());
    if (CV_r_glow && CV_r_useparticles_glow && pTech && pTech->m_nTechnique[TTYPE_GLOWPASS] > 0 && shaderItem.m_pShaderResources && ((SRenderShaderResources *)shaderItem.m_pShaderResources)->Glow())
      nBatchFlags |= FB_GLOW;

    //if ( CV_r_usewaterparticles  && shaderItem.m_pShader && (((CShader *)shaderItem.m_pShader)->m_Flags & EF_WATERPARTICLE) )
    //{
    //  nBatchFlags = FB_GENERAL;
    //  nList = EFSLIST_WATERPARTICLES;
    //}

    SRendItem::mfAdd(m_pRT->GetThreadList(), pParticle, pRO, shaderItem, nList, nAW, nBatchFlags);    
  }
}

void CRenderer::EF_AddClientPolys()
{
  uint32 i;
  CREClientPoly *pl;

  ASSERT_IS_MAIN_THREAD(m_pRT)
  int nThreadID = m_pRT->GetThreadList();
	int nR = SRendItem::m_RecurseLevel[nThreadID]-1;
	const SThreadInfo& rTI = m_RP.m_TI[nThreadID];
	const uint32 nPersFlags = rTI.m_PersFlags;

  for (i=0; i<CREClientPoly::m_PolysStorage[nThreadID][nR].Num(); i++)
  {
    pl = CREClientPoly::m_PolysStorage[nThreadID][nR][i];

    if (pl->m_Shader.m_nPreprocessFlags)
      SRendItem::mfAdd(nThreadID, pl, pl->m_pObject, pl->m_Shader, EFSLIST_PREPROCESS, 0, FB_GENERAL);
    if (pl->m_Shader.m_pShader->GetFlags() & EF_DECAL) // || pl->m_pObject && (pl->m_pObject->m_ObjFlags & FOB_DECAL))
    {
      CShader *pS = (CShader *)pl->m_Shader.m_pShader;
      SRenderShaderResources *const __restrict pR = (SRenderShaderResources *)pl->m_Shader.m_pShaderResources;
      SShaderTechnique *pTech = pl->m_Shader.GetTechnique();

      uint32 nBatchFlags = FB_GENERAL;
      if( CV_r_deferredshading )
      {
        if( pTech && pTech->m_nTechnique[TTYPE_Z] > 0 && (pS && (pS->m_Flags & EF_SUPPORTSDEFERREDSHADING) ) )
        {
          // temporary workaround: if no normal map use base normal from NormalsTarget          
          // todo: make this instead by decal type
          if( pl->m_Shader.m_pShaderResources && pl->m_Shader.m_pShaderResources->GetTexture(EFTT_BUMP) || CV_r_deferredshading ==3)         
            nBatchFlags |= FB_Z;
        }
      }

      if (pTech && pTech->m_nTechnique[TTYPE_GLOWPASS] > 0 && pR->Glow())
        nBatchFlags |= FB_GLOW;

			if(!(nPersFlags & RBPF_SHADOWGEN) && !(pl->m_nCPFlags & CREClientPoly::efShadowGen))
				SRendItem::mfAdd(nThreadID, pl, pl->m_pObject, pl->m_Shader, EFSLIST_DECAL, pl->m_nCPFlags & CREClientPoly::efAfterWater, nBatchFlags);
			else if((nPersFlags & RBPF_SHADOWGEN) && (pl->m_nCPFlags & CREClientPoly::efShadowGen))
				SRendItem::mfAdd(nThreadID, pl, pl->m_pObject, pl->m_Shader, EFSLIST_SHADOW_GEN, SG_SORT_GROUP, FB_GENERAL);
    }
    else
      SRendItem::mfAdd(nThreadID, pl, pl->m_pObject, pl->m_Shader, EFSLIST_GENERAL, pl->m_nCPFlags & CREClientPoly::efAfterWater, FB_GENERAL);
  }
}


// Dynamic lights
bool CRenderer::EF_IsFakeDLight(const CDLight *Source)
{
  if (!Source)
  {
    iLog->Log("Warning: EF_IsFakeDLight: NULL light source\n");
    return true;
  }

  bool bIgnore = false;
  if (Source->m_Flags & (DLF_DEFERRED_LIGHT|DLF_FAKE))
    bIgnore = true;

  return bIgnore;
}

void CRenderer::EF_CheckLightMaterial( CDLight *Source, CDLight *pNewLight)
{
///* //Diesel cut
	int nThreadID = m_RP.m_nFillThreadID;
	int nR = SRendItem::m_RecurseLevel[nThreadID]-1;

	if (!(m_RP.m_TI[nThreadID].m_PersFlags2 & RBPF2_IMPOSTERGEN))
	{
		// Add light coronas, lens flares, beams and so on (depends on shader)
		if (Source->m_Shader.m_pShader && Source->m_Shader.m_pShader->GetREs(Source->m_Shader.m_nTechnique) && Source->m_Shader.m_pShader->GetREs(Source->m_Shader.m_nTechnique)->Num())
		{
			I3DEngine *eng = (I3DEngine *)gEnv->p3DEngine;
			float fWaterLevel = eng->GetWaterLevel();
			float fCamZ = m_RP.m_TI[nThreadID].m_cam.GetPosition().z;
			//get the permanent object
			if (!Source->m_pObject[nR])
			{         
				Source->m_pObject[nR] = EF_GetObject(false, -1);
				Source->m_pObject[nR]->m_fAlpha = 0;
				Source->m_pObject[nR]->m_II.m_AmbColor = Vec3(0,0,0);
				SRenderObjData *pOD = EF_GetObjData(Source->m_pObject[nR], true);
				pOD->m_fTempVars[0] = 0;
				pOD->m_fTempVars[1] = 0;
				pOD->m_fTempVars[3] = Source->m_fRadius;
			}
			else
				EF_GetObject(false, Source->m_pObject[nR]->m_Id);	

			if (pNewLight)
				pNewLight->m_pObject[nR] = Source->m_pObject[nR];

			SRenderObjData *pOD = EF_GetObjData(Source->m_pObject[nR], true);

			CRendElementBase *pRE = Source->m_Shader.m_pShader->GetREs(Source->m_Shader.m_nTechnique)->Get(0);
			float fCustomSort = 0;
			if (pRE->mfGetType() == eDATA_Flare)
			{          
				Source->m_pObject[nR]->m_II.m_AmbColor.r = Source->m_Color.r;
				Source->m_pObject[nR]->m_II.m_AmbColor.g = Source->m_Color.g;
				Source->m_pObject[nR]->m_II.m_AmbColor.b = Source->m_Color.b;
				fCustomSort = 4000.0f;
			}
			else
			{
				Source->m_pObject[nR]->m_II.m_AmbColor = Source->m_Color;
				pOD->m_fTempVars[3] = Source->m_fRadius;
			}

			Source->m_pObject[nR]->m_II.m_Matrix = Source->m_ObjMatrix;
			Source->m_pObject[nR]->m_pLight = Source;
			Source->m_pObject[nR]->m_DynLMMask[nThreadID] = 1<<Source->m_Id;
			Source->m_pObject[nR]->m_ObjFlags |= FOB_TRANS_MASK;

			UINT nList = EFSLIST_TRANSP;
			int nAW = 0;
			if((fCamZ-fWaterLevel)*(Source->m_Origin.z-fWaterLevel)>0)
				nAW = 1;
			else
				nAW = 0;
			EF_AddEf(pRE, Source->m_Shader, Source->m_pObject[nR], nList, nAW);
		}
	}
//*/ //Diesel cut
}

void CRenderer::EF_ADDDlight(CDLight *Source)
{
  if (!Source)
  {
    iLog->Log("Warning: EF_ADDDlight: NULL light source\n");
    return;
  }

  ASSERT_IS_MAIN_THREAD(m_pRT)

  bool bIgnore = EF_IsFakeDLight(Source);
  //Source->m_Flags &= ~DLF_POINT;
  //Source->m_Flags |= DLF_DIRECTIONAL;

  int nThreadID = m_RP.m_nFillThreadID;
  int nR = SRendItem::m_RecurseLevel[nThreadID]-1;

  CDLight *pNew = NULL;
  if (bIgnore)
    Source->m_Id = -1;
  else
  {
    assert((Source->m_Flags & DLF_LIGHTTYPE_MASK) != 0);
    Source->m_Id = m_RP.m_DLights[nThreadID][nR].Num();
    if (Source->m_Id >= 32)
    {
      //iLog->Log("Warning: EF_ADDDlight: Too many light sources (Ignored)\n");
      Source->m_Id = -1;
      return;
    }
    pNew = m_RP.m_DLights[nThreadID][nR].AddIndex(1);
    memcpy(pNew, Source, sizeof(CDLight));
  }
  EF_PrecacheResource(Source, (m_RP.m_TI[nThreadID].m_cam.GetPosition()-Source->m_Origin).GetLengthSquared() / max(0.001f, Source->m_fRadius * Source->m_fRadius), 0.1f, 0, 0);

  EF_CheckLightMaterial(Source, pNew);
}

void CRenderer::EF_AddDeferrredDecal(const SDeferrredDecal & rDecal)
{
  ASSERT_IS_MAIN_THREAD(m_pRT)

  int nThreadID = m_RP.m_nFillThreadID;
  int nR = SRendItem::m_RecurseLevel[nThreadID]-1;

  if(m_RP.m_DeferrredDecals[nThreadID][nR].Num()<512)
    m_RP.m_DeferrredDecals[nThreadID][nR].AddElem(rDecal);
}

void CRenderer::EF_ClearLightsList()
{
  ASSERT_IS_MAIN_THREAD(m_pRT)
  m_RP.m_DLights[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]-1].SetUse(0);
  m_RP.m_SMFrustums[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]-1].SetUse(0);
}

inline Matrix44	ToLightMatrix(const Ang3 &angle)	{
	Matrix33 ViewMatZ=Matrix33::CreateRotationZ(-angle.x);
	Matrix33 ViewMatX=Matrix33::CreateRotationX(-angle.y);
	Matrix33 ViewMatY=Matrix33::CreateRotationY(+angle.z);
	return GetTransposed44(Matrix44(ViewMatX*ViewMatY*ViewMatZ));
}

/* //Diesel cut
bool CRenderer::EF_UpdateDLight(CDLight *dl)
{
  bool bRes = false;
  if (!dl)
    return bRes;

  float fTime = iTimer->GetCurrTime();			// todo: light does not need to use float time - better: renderframeid

  bRes = true;


  SShaderTechnique *pTech = dl->m_Shader.GetTechnique();
  uint32 nStyle = dl->m_nLightStyle;
  if (nStyle>0 && nStyle<CLightStyle::m_LStyles.Num() && CLightStyle::m_LStyles[nStyle])
  {
    CLightStyle *ls = CLightStyle::m_LStyles[nStyle];
    ls->mfUpdate(fTime);
    dl->m_Color = dl->m_BaseColor * ls->m_fIntensity;
  }
	else

	{ 
		dl->m_Color = dl->m_BaseColor;
	} 

	if(EF_Query(EFQ_HDRModeEnabled)!=0)
	{
		I3DEngine *p3DEngine = (I3DEngine *)gEnv->p3DEngine;		assert(p3DEngine);

		float fHDR = powf( p3DEngine->GetHDRDynamicMultiplier(), dl->m_fHDRDynamic );
  	dl->m_Color *= fHDR;
	}  

  return false;
}
*/ //Diesel cut

//Diesel new
///*
bool CRenderer::EF_UpdateDLight(CDLight *dl)
{
  bool bRes = false;
  if (!dl)
    return bRes;
  float fTime = iTimer->GetCurrTime();			// todo: light does not need to use float time - better: renderframeid
  bRes = true;
	dl->m_Color = dl->m_BaseColor;
  return false;
}
//*/ //Diesel new

void CRenderer::FX_ApplyShaderQuality(EShaderType eST)
{
  SShaderProfile *pSP = &m_cEF.m_ShaderProfiles[eST];
  m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1]);
  int nQuality = (int)pSP->GetShaderQuality();
  m_RP.m_nShaderQuality = nQuality;
  switch (nQuality)
  {
    case eSQ_Medium:
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY];
      break;
    case eSQ_High:
    case eSQ_VeryHigh:
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY1];
      break;
  }
}

EShaderQuality CRenderer::EF_GetShaderQuality(EShaderType eST)
{
  SShaderProfile *pSP = &m_cEF.m_ShaderProfiles[eST];
  int nQuality = (int)pSP->GetShaderQuality();

  switch (nQuality)
  {
    case eSQ_Low:
      return eSQ_Low;
    case eSQ_Medium:
      return eSQ_Medium;
    case eSQ_High:
    case eSQ_VeryHigh:
      return eSQ_High;
  }

  return eSQ_Low;
}

ERenderQuality CRenderer::EF_GetRenderQuality() const
{
  return (ERenderQuality) m_RP.m_eQuality;
}

#ifdef WIN64
#pragma warning( push )							//AMD Port
#pragma warning( disable : 4312 )				// 'type cast' : conversion from 'int' to 'void *' of greater size
#endif
 
int CRenderer::RT_CurThreadList()
{
  return m_pRT->GetThreadList();
}

void *CRenderer::EF_Query(int Query, INT_PTR Param)
{
  static int nSize;

  switch (Query)
  {
    case EFQ_DeleteMemoryArrayPtr:
      {
        char *pPtr = (char *)Param;
        delete [] pPtr;
      }
      break;
    case EFQ_DeleteMemoryPtr:
      {
        char *pPtr = (char *)Param;
        delete pPtr;
      }
      break;
    case EFQ_GetShaderCombinations:
      {
        const char *szPtr = CHWShader::GetCurrentShaderCombinations(true);
        return (void *)szPtr;
      }
      break;
    case EFQ_SetShaderCombinations:
      {
        if (CV_r_shaderspreactivate)
        {
          iLog->Log("--- Preactivate shaders...");
          iLog->UpdateLoadingScreen(0);
          bool bRes = CHWShader::SetCurrentShaderCombinations((const char *)Param, true);
				  m_cEF.m_bActivated = true;
        }
        return NULL;
      }
      break;
    case EFQ_CloseShaderCombinations:
      {
        m_cEF.mfCloseShadersCache(0);
        return NULL;
      }
      break;
    case EFQ_LightSource:
      {
        if (m_RP.m_DLights[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]-1].Num() > (uint32)Param)
          return &m_RP.m_DLights[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]-1][Param];
        return NULL;
      }
      break;

    case EFQ_ShaderGraphBlocks:
      return (void *)&m_cEF.m_GR.m_Blocks;

    case EFQ_MainThreadList:
      return (void *)m_RP.m_nFillThreadID;
    case EFQ_RenderThreadList:
      return (void *)m_RP.m_nProcessThreadID;
    case EFQ_RenderMultithreaded:
      nSize = m_pRT->IsMultithreaded();
      return (void *)nSize;
      break;

    case EFQ_Pointer2FrameID:
      return (void *)&m_RP.m_TI[m_pRT->GetThreadList()].m_nFrameID;
    case EFQ_D3DDevice:
      return gGet_D3DDevice();
    case EFQ_glReadPixels:
      return gGet_glReadPixels();
    case EFQ_DeviceLost:
      nSize = (int)CheckDeviceLost();
      return (void *)&nSize;
    case EFQ_RecurseLevel:
      return (void *)(UINT_PTR)SRendItem::m_RecurseLevel[m_pRT->GetThreadList()];
    case EFQ_Alloc_APITextures:
      {
        nSize = 0;
        SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        if (pRL)
        {
          ResourcesMapItor itor;
          for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
          {
            CTexture *tp = (CTexture *)itor->second;
            if (!tp || tp->IsNoTexture())
              continue;
#if !defined(XENON) && !defined(PS3)
            if (!(tp->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET)))
              nSize += tp->GetDeviceDataSize();


#endif
          }
        }
        return (void *)&nSize;
      }
      break;
		case EFQ_PS3_Resource_Stats:
			{
				void* pData	=	0;



        return pData;
			}
			break;
    case EFQ_Alloc_APIMesh:
      {
        nSize = 0;
        CRenderMesh2 *pRM;
        // Vertices
        for (pRM=CRenderMesh2::m_Root.m_Next; pRM!=&CRenderMesh2::m_Root; pRM=pRM->m_Next)
        {
          nSize += pRM->Size(1);
					nSize += pRM->Size(2);
        }
        return (void *)&nSize;
      }
      break;
		
		case EFQ_Alloc_Mesh_SysMem:
			{
				nSize = 0;
				CRenderMesh2 *pRM;
				for (pRM=CRenderMesh2::m_Root.m_Next; pRM!=&CRenderMesh2::m_Root; pRM=pRM->m_Next)
				{
					nSize += pRM->Size(0);
				}
				return (void *)&nSize;
			}
			break;
		
		case EFQ_Mesh_Count:
			{
				nSize = 0;
				CRenderMesh2 *pRM;
				// Vertices
				for (pRM=CRenderMesh2::m_Root.m_Next; pRM!=&CRenderMesh2::m_Root; pRM=pRM->m_Next)
				{
					nSize++;
				}
				return (void *)&nSize;
			}
			break;

		case EFQ_GetAllMeshes:
			{
				IRenderMesh **pMeshes = (IRenderMesh**)Param;
				nSize = 0;
				CRenderMesh2 *pRM;
				// Vertices
				for (pRM=CRenderMesh2::m_Root.m_Next; pRM!=&CRenderMesh2::m_Root; pRM=pRM->m_Next)
				{
					if (pMeshes)
					{
						pMeshes[nSize] = pRM;
						//CryLog("Still alive %s", pRM->GetSourceName());
					}
					nSize++;
				}
				return (void *)&nSize;
			}
			break;

		case EFQ_GetAllTextures:
			{
				nSize = 0;
				ITexture **pTextures = (ITexture**)Param;
				SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
				if (pRL)
				{
					ResourcesMapItor itor;
					for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
					{
						CTexture *tp = (CTexture *)itor->second;
						if (!tp || tp->IsNoTexture())
							continue;
						if (pTextures)
						{
							pTextures[nSize] = tp;
						}
						nSize++;
					}
				}
				return (void *)&nSize;
			}
			break;
		case EFQ_TexturesPoolSize:
			nSize = (CRenderer::CV_r_texturesstreampoolsize + 2) * 1024 * 1024;
			return (void *)&nSize;
			break;
		case EFQ_HDRModeEnabled:
			return IsHDRModeEnabled() ? (void *)1 : 0;

		case EFQ_MultiGPUEnabled:
			return IsMultiGPUModeActive() ? (void *)1 : 0;

		case EFQ_sLinearSpaceShadingEnabled:
			return IsLinearSpaceShadingEnabled() ? (void *)1 : 0;		// sRGB

		case EFQ_DrawNearFov:
			{
				static float prevValue;
				prevValue = CV_r_drawnearfov;
				if (Param)
					CV_r_drawnearfov = *(float*) Param;
				return (void*) &prevValue;
			}

		case EFQ_TextureStreamingEnabled:
			return CTexture::s_nStreamingMode ? (void *)1 : 0;

		case EFQ_FSAAEnabled:
			return m_RP.IsFSAAEnabled() ? (void *)1 : 0;

		case EFQ_Fullscreen:
			return m_FullScreen ? (void *)1 : 0;

		case EFQ_GetTexStreamingInfo:
			{
				STextureStreamingStats* stats = (STextureStreamingStats*)Param;
				if(stats)
				{
					stats->nCurrentPoolSize = CTexture::s_nStatsCurManagedStreamedTexMem;
					stats->bPoolOverflow = CTexture::s_bPoolOverflow;
					stats->nMaxPoolSize = CRenderer::CV_r_texturesstreampoolsize * 1024 * 1024;
					stats->nThroughput =  (CTexture::s_nStreamingTotalTime > 0.f) ? uint32((float)CTexture::s_nStreamingThroughput / CTexture::s_nStreamingTotalTime) : 0;
				}
				return (void *)(CTexture::s_nStreamingTotalTime > 0.f && stats != NULL);
			}

		case EFQ_HalfPixelShiftNeeded:
			{
			#if HALF_PIXEL_SHIFT_NEEDED
				return (void*)true;
			#else
				return 0;
			#endif
			}

		case EFQ_OverscanBorders:
			{
				return (void*) &s_overscanBorders;
			}
		
		case EFQ_NumActivePostEffects:
			{
				nSize = 0;
				if(CV_r_PostProcess)
				{
					//assume query is from main thread
					nSize = PostEffectMgr().GetActiveEffects(m_RP.m_nFillThreadID).size();
				}

				if( CV_r_SSAO )
				{
					nSize++;
				}

				return (void*)&nSize;		
			}
			
		default:
      assert(0);
  }
  return NULL;
}

#ifdef WIN64
#pragma warning( pop )							//AMD Port
#endif

//================================================================================================================

IRenderMesh * CRenderer::CreateRenderMesh(const char *szType, const char *szSourceName)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", szSourceName);
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", szType);

  // make material table with clean elements
  CRenderMesh2 *pRenderMesh = new CRenderMesh2(szType,szSourceName);
  pRenderMesh->AddRef();

  return pRenderMesh;
}

void CRenderer::DeleteRenderMesh(IRenderMesh * pLBuffer)
{
  if(pLBuffer)
	{
		pLBuffer->Release();
	}
}

// Creates the RenderMesh with the materials, secondary buffer (system buffer)
// indices and perhaps some other stuff initialized.
// NOTE: if the pVertBuffer is NULL, the system buffer doesn't get initialized with any values
// (trash may be in it)
IRenderMesh * CRenderer::CreateRenderMeshInitialized(
  void * pVertBuffer, int nVertCount, EVertexFormat eVF, 
  uint16 * pIndices, int nIndices,
  int nPrimetiveType, const char *szType, const char *szSourceName, ERenderMeshType eBufType,
  int nMatInfoCount, int nClientTextureBindID,
  bool (*PrepareBufferCallback)(IRenderMesh *, bool),
  void *CustomData,bool bOnlyVideoBuffer, bool bPrecache, 
	SPipTangents * pTangents)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", szSourceName);
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", szType);

  CRenderMesh2 * pRenderMesh = new CRenderMesh2(szType,szSourceName);
	pRenderMesh->AddRef();
 
  // make mats info list
  pRenderMesh->m_Chunks.PreAllocate(nMatInfoCount);
  pRenderMesh->_SetVertexFormat(eVF);
  pRenderMesh->_SetNumVerts(nVertCount);

  // copy vert buffer
  if (pVertBuffer && !PrepareBufferCallback && !bOnlyVideoBuffer)
  {
    pRenderMesh->UpdateVertices(pVertBuffer, nVertCount, 0, VSF_GENERAL);

		if (pTangents)
      pRenderMesh->UpdateVertices(pTangents, nVertCount, 0, VSF_TANGENTS);
  }

	pRenderMesh->m_pCustomData = CustomData;
  pRenderMesh->_SetNumInds(nIndices);
  if (pIndices)
    pRenderMesh->UpdateIndices(pIndices, nIndices);
  pRenderMesh->_SetPrimitiveType(nPrimetiveType);
  pRenderMesh->_SetRenderMeshType(eBufType);

  pRenderMesh->m_nClientTextureBindID = nClientTextureBindID;

  // Precache for static buffers
  if (CV_r_meshprecache && pRenderMesh->_GetNumVerts() && bPrecache && !m_bDeviceLost && m_pRT->IsRenderThread())
  {
    pRenderMesh->CheckUpdate(eVF, 0);
#ifndef NULL_RENDERER
    assert (pRenderMesh->_HasVBStream(VSF_GENERAL));
#endif
  }
  
  return pRenderMesh;
}

//=======================================================================

#define MINGEOMAREA 10e-6
#define MINTEXAREA 10e-8

const float CRenderer::CalculateTexelAreaDensity(CMesh &mesh, const int subMeshIndex, const char* sMeshName)
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );

	assert(0 <= subMeshIndex && subMeshIndex < mesh.GetSubSetCount());
	
	assert(sMeshName);

	bool bCorruptedMeshIndices = false;

	if (0 <= subMeshIndex && subMeshIndex < mesh.GetSubSetCount())
	{
		if (mesh.m_pTexCoord != NULL)
		{
			SMeshSubset &subset = mesh.m_subsets[subMeshIndex];
			float texAreaSum = 0.0f;
			float posAreaSum = 0.0f;
			float posAreaTotal = 0.0f;
			int posAreaTotalCount = 0;

			assert(subset.nNumIndices % 3 == 0);

			for (int i = 0; i < subset.nNumIndices; i+=3)
			{
				const uint16 index0 = mesh.m_pIndices[subset.nFirstIndexId+i];
				const uint16 index1 = mesh.m_pIndices[subset.nFirstIndexId+i+1];
				const uint16 index2 = mesh.m_pIndices[subset.nFirstIndexId+i+2];

				if(index0 >= mesh.m_numVertices || index1 >= mesh.m_numVertices || index2 >= mesh.m_numVertices)
				{
					bCorruptedMeshIndices = true;
					continue;
				}

				const Vec3& pos0 = mesh.m_pPositions[index0];
				const Vec3& pos1 = mesh.m_pPositions[index1];
				const Vec3& pos2 = mesh.m_pPositions[index2];

				Vec2 tex0;
				tex0.x = mesh.m_pTexCoord[index0].s;
				tex0.y = mesh.m_pTexCoord[index0].t;
				Vec2 tex1;
				tex1.x = mesh.m_pTexCoord[index1].s;
				tex1.y = mesh.m_pTexCoord[index1].t;
				Vec2 tex2;
				tex2.x = mesh.m_pTexCoord[index2].s;
				tex2.y = mesh.m_pTexCoord[index2].t;

				const float posArea = ((pos1-pos0).Cross(pos2-pos0)).GetLength();
				const float texArea = fabsf((tex1-tex0).Cross(tex2-tex0));

				if (posArea > MINGEOMAREA && texArea > MINTEXAREA)
				{
					posAreaTotal += posArea;
					posAreaTotalCount++;
				}
			}

			if (posAreaTotalCount != 0)
			{
				posAreaTotal /= posAreaTotalCount;
			}
			else
			{
				// if the area of the mesh in 3d space or uv space is 0, use a big number to ignore this mesh in streaming calculations
				// (this value used in further calculations, so FLT_MAX in not valid here)
				return (float)UINT_MAX;
			}

			for (int i = 0; i < subset.nNumIndices; i+=3)
			{
				const uint16 index0 = mesh.m_pIndices[subset.nFirstIndexId+i];
				const uint16 index1 = mesh.m_pIndices[subset.nFirstIndexId+i+1];
				const uint16 index2 = mesh.m_pIndices[subset.nFirstIndexId+i+2];

				if(index0 >= mesh.m_numVertices || index1 >= mesh.m_numVertices || index2 >= mesh.m_numVertices)
				{
					bCorruptedMeshIndices = true;
					continue;
				}

				const Vec3& pos0 = mesh.m_pPositions[index0];
				const Vec3& pos1 = mesh.m_pPositions[index1];
				const Vec3& pos2 = mesh.m_pPositions[index2];

				const float posArea = ((pos1-pos0).Cross(pos2-pos0)).GetLength();

				if (posArea >= posAreaTotal)
				{
					Vec2 tex0;
					tex0.x = mesh.m_pTexCoord[index0].s;
					tex0.y = mesh.m_pTexCoord[index0].t;
					Vec2 tex1;
					tex1.x = mesh.m_pTexCoord[index1].s;
					tex1.y = mesh.m_pTexCoord[index1].t;
					Vec2 tex2;
					tex2.x = mesh.m_pTexCoord[index2].s;
					tex2.y = mesh.m_pTexCoord[index2].t;

					const float texArea = fabsf((tex1-tex0).Cross(tex2-tex0));

					if (texArea > MINTEXAREA)
					{
						texAreaSum += texArea;
						posAreaSum += posArea;
					}
				}
			}

			if (posAreaSum > MINGEOMAREA)
			{
				texAreaSum /= posAreaSum;
			}

			if(bCorruptedMeshIndices)
			{
				assert(0);
				gEnv->pLog->LogError("CRenderer::CalculateTexelAreaDensity: Mesh '%s' has corrupted indices!", sMeshName);
			}

			return texAreaSum;
		}
		else
		{
			return 1.0f;
		}
	}

	return 0;
}

const float CRenderer::CalculateTexelAreaDensity(const PodArray<SVF_P3F_C4B_T2F> &vertices,const PodArray<unsigned short> &indices, const char* sMeshName)
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );
	
	assert(sMeshName);

	float texAreaSum = 0.0f;
	float posAreaSum = 0.0f;
	float posAreaTotal = 0.0f;
	int posAreaTotalCount = 0;

	const size_t nVers = vertices.size();
	bool bCorruptedIndices = false;

	for (int i = 0; i < indices.size()/3; i++)
	{
		const unsigned short index0 = indices[i*3];
		const unsigned short index1 = indices[i*3+1];
		const unsigned short index2 = indices[i*3+2];

		if(index0 >= nVers || index1 >= nVers || index2 >= nVers)
		{
			bCorruptedIndices = true;
			continue;
		}

		const Vec3 pos0 = vertices[index0].xyz;
		const Vec2 tex0 = vertices[index0].st;

		const Vec3 pos1 = vertices[index1].xyz;
		const Vec2 tex1 = vertices[index1].st;

		const Vec3 pos2 = vertices[index2].xyz;
		const Vec2 tex2 = vertices[index2].st;

		const float posArea = ((pos1-pos0).Cross(pos2-pos0)).GetLength();
		const float texArea = fabsf((tex1-tex0).Cross(tex2-tex0));

		if (posArea > MINGEOMAREA && texArea > MINTEXAREA)
		{
			posAreaTotal += posArea;
			posAreaTotalCount++;
		}
	}

	if (posAreaTotalCount != 0)
	{
		posAreaTotal /= posAreaTotalCount;
	}
	else
	{
		// if the area of the mesh in 3d space or uv space is 0, use a big number to ignore this mesh in streaming calculations
		// (this value used in further calculations, so FLT_MAX in not valid here)
		return (float)UINT_MAX;
	}

	for (int i = 0; i < indices.size()/3; i++)
	{
		const unsigned short index0 = indices[i*3];
		const unsigned short index1 = indices[i*3+1];
		const unsigned short index2 = indices[i*3+2];

		if(index0 >= nVers || index1 >= nVers || index2 >= nVers)
		{
			bCorruptedIndices = true;
			continue;
		}

		const Vec3 pos0 = vertices[index0].xyz;
		const Vec3 pos1 = vertices[index1].xyz;
		const Vec3 pos2 = vertices[index2].xyz;

		const float posArea = ((pos1-pos0).Cross(pos2-pos0)).GetLength();

		if (posArea >= posAreaTotal)
		{
			const Vec2 tex0 = vertices[index0].st;
			const Vec2 tex1 = vertices[index1].st;
			const Vec2 tex2 = vertices[index2].st;

			const float texArea = fabsf((tex1-tex0).Cross(tex2-tex0));

			if (texArea > MINTEXAREA)
			{
				texAreaSum += texArea;
				posAreaSum += posArea;
			}
		}
	}

	if(bCorruptedIndices)
	{
		assert(0);
		gEnv->pLog->LogError("CRenderer::CalculateTexelAreaDensity: Index stream of mesh '%s' has corrupted indices!", sMeshName);
	}

	if (posAreaSum > MINGEOMAREA)
	{
		texAreaSum /= posAreaSum;
	}

	return texAreaSum;
}

const float CRenderer::CalculateTexelAreaDensity(const PodArray<SVF_P3S_C4B_T2S> &vertices,const PodArray<unsigned short> &indices, const char* sMeshName)
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );

	assert(sMeshName);

	float texAreaSum = 0.0f;
	float posAreaSum = 0.0f;
	float posAreaTotal = 0.0f;
	int posAreaTotalCount = 0;

	const size_t nVers = vertices.size();
	bool bCorruptedIndices = false;

	for (int i = 0; i < indices.size()/3; i++)
	{
		const unsigned short index0 = indices[i*3];
		const unsigned short index1 = indices[i*3+1];
		const unsigned short index2 = indices[i*3+2];

		if(index0 >= nVers || index1 >= nVers || index2 >= nVers)
		{
			bCorruptedIndices = true;
			continue;
		}

		const Vec3 pos0 = vertices[index0].xyz.ToVec3();
		const Vec2 tex0 = vertices[index0].st.ToVec2();

		const Vec3 pos1 = vertices[index1].xyz.ToVec3();
		const Vec2 tex1 = vertices[index1].st.ToVec2();

		const Vec3 pos2 = vertices[index2].xyz.ToVec3();
		const Vec2 tex2 = vertices[index2].st.ToVec2();

		const float posArea = ((pos1-pos0).Cross(pos2-pos0)).GetLength();
		const float texArea = fabsf((tex1-tex0).Cross(tex2-tex0));

		if (posArea > MINGEOMAREA && texArea > MINTEXAREA)
		{
			posAreaTotal += posArea;
			posAreaTotalCount++;
		}
	}

	if (posAreaTotalCount != 0)
	{
		posAreaTotal /= posAreaTotalCount;
	}
	else
	{
		// if the area of the mesh in 3d space or uv space is 0, use a big number to ignore this mesh in streaming calculations.
		// (this value used in further calculations, so FLT_MAX in not valid here)
		return (float)UINT_MAX;
	}

	for (int i = 0; i < indices.size()/3; i++)
	{
		const unsigned short index0 = indices[i*3];
		const unsigned short index1 = indices[i*3+1];
		const unsigned short index2 = indices[i*3+2];

		if(index0 >= nVers || index1 >= nVers || index2 >= nVers)
		{
			bCorruptedIndices = true;
			continue;
		}

		const Vec3 pos0 = vertices[index0].xyz.ToVec3();
		const Vec3 pos1 = vertices[index1].xyz.ToVec3();
		const Vec3 pos2 = vertices[index2].xyz.ToVec3();

		const float posArea = ((pos1-pos0).Cross(pos2-pos0)).GetLength();

		if (posArea >= posAreaTotal)
		{
			const Vec2 tex0 = vertices[index0].st.ToVec2();
			const Vec2 tex1 = vertices[index1].st.ToVec2();
			const Vec2 tex2 = vertices[index2].st.ToVec2();

			const float texArea = fabsf((tex1-tex0).Cross(tex2-tex0));

			if (texArea > MINTEXAREA)
			{
				texAreaSum += texArea;
				posAreaSum += posArea;
			}
		}
	}

	if(bCorruptedIndices)
	{
		assert(0);
		gEnv->pLog->LogError("CRenderer::CalculateTexelAreaDensity: Index stream of mesh '%s' has corrupted indices!", sMeshName);
	}

	if (posAreaSum > MINGEOMAREA)
	{
		texAreaSum /= posAreaSum;
	}

	return texAreaSum;
}

//=======================================================================

void CRenderer::SetWhiteTexture()
{
  m_pRT->RC_SetTexture(CTexture::s_ptexWhite->GetID(), 0);
}

void CRenderer::SetTexture(int tnum)
{
  m_pRT->RC_SetTexture(tnum, 0);
}

// used for sprite generation
void CRenderer::SetTextureAlphaChannelFromRGB(byte * pMemBuffer, int nTexSize)
{
#ifndef XENON
  // set alpha channel
  for(int y=0; y<nTexSize; y++)
    for(int x=0; x<nTexSize; x++)
    {
      int t = (x+nTexSize*y)*4;
      if( abs(pMemBuffer[t+0]-pMemBuffer[0+0])<2 && 
          abs(pMemBuffer[t+1]-pMemBuffer[0+1])<2 && 
          abs(pMemBuffer[t+2]-pMemBuffer[0+2])<2 )
        pMemBuffer[t+3] = 0;
      else
        pMemBuffer[t+3] = 255;

      // set border alpha to 0
      if( x==0 || y == 0 || x == nTexSize-1 || y == nTexSize-1 )
        pMemBuffer[t+3] = 0;
    }



















#endif
}

//=============================================================================
// Precaching
bool CRenderer::EF_PrecacheResource(IRenderMesh *_pPB, float fMipFactor, float fTimeToReady, int nFlags, int nUpdateId)
{
  int i;
  if (!CTexture::s_nStreamingEnabled)
    return true;

	CRenderMesh2 * pPB = (CRenderMesh2 *)_pPB;

  for (i=0; i<pPB->m_Chunks.Count(); i++)
  {
    CRenderChunk *pChunk = &pPB->m_Chunks[i];
		assert(!"do pre-cache with real materials");

		assert(0);

		//@TODO: Timur
		assert( pPB->GetMaterial() && "RenderMesh must have material" );
    SRenderShaderResources *pSR = (SRenderShaderResources *)pPB->GetMaterial()->GetShaderItem(pChunk->m_nMatID).m_pShaderResources;
    if (!pSR)
      continue;
    if (pSR->m_nFrameLoad != m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameID)
    {
      pSR->m_nFrameLoad = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameID;
      pSR->m_fMinMipFactorLoad = 999999.0f;
    }
    else
      if (fMipFactor >= pSR->m_fMinMipFactorLoad || CTexture::s_nStreamingEnabled == 2)
      continue;
    pSR->m_fMinMipFactorLoad = fMipFactor;
    for (int j=0; j<=pSR->m_nLastTexture; j++)
    {
      if (!pSR->m_Textures[j])
        continue;
      CTexture *tp = pSR->m_Textures[j]->m_Sampler.m_pTex;
      if (!tp)
        continue;
			fMipFactor *= pSR->m_Textures[j]->m_TexModificator.m_Tiling[0] * pSR->m_Textures[j]->m_TexModificator.m_Tiling[1];
      tp->PrecacheAsynchronously(fMipFactor, nFlags, nUpdateId);
    }
  }
  return true;
}

bool CRenderer::EF_PrecacheResource(CDLight *pLS, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId)
{
  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_RENDERER,g_bProfilerEnabled );

  if (!CTexture::s_nStreamingEnabled)
    return true;

  if (pLS->m_pLightImage)
    pLS->m_pLightImage->PrecacheAsynchronously(fMipFactor, Flags, nUpdateId);
	if (pLS->GetDiffuseCubemap())
		pLS->GetDiffuseCubemap()->PrecacheAsynchronously(fMipFactor, Flags, nUpdateId);
	if (pLS->GetSpecularCubemap())
		pLS->GetSpecularCubemap()->PrecacheAsynchronously(fMipFactor, Flags, nUpdateId);
  return true;
}

void CRenderer::PrecacheTexture(ITexture *pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId)
{
  if (!CTexture::s_nStreamingEnabled)
    return;

	assert(m_pRT->IsRenderThread());

  if (pTP)
    pTP->PrecacheAsynchronously(fMipFactor, Flags, nUpdateId);
}

bool CRenderer::EF_PrecacheResource(IShader *pSH, float fMipFactor, float fTimeToReady, int Flags)
{
  if (!CTexture::s_nStreamingEnabled)
    return true;

  return true;
}

#if !defined(XENON) && !defined(PS3)

//#include "Textures/Image/NVDXT/dxtlib.h" //Diesel cut
#include "../../../SDKs/nvDXTLibrary/nvDXTLibrary.h" //Diesel new

extern  byte *sDDSData;
///* //Diesel cut
NV_ERROR_CODE ReadDTXnFile(void *buffer, size_t count, void * userData)
{
	cryMemcpy(buffer, sDDSData, count);
	sDDSData += count;

	return NV_OK;
}
//*/ //Diesel cut
#endif


void CRenderer::DXTCompress_FastOnCPU( byte * pInData, byte * pOutData, const int width, const int height, const bool isDXT5 )
{
  const size_t mipSize = ((width + 3) / 4) * ((height + 3) / 4) * (isDXT5 ? 16 : 8);

  int size;
  RTDXTCompression::CompressImageDXT(pInData, pOutData, width, height, size, isDXT5);
  if(mipSize != size)
  {
    assert(0);
    gEnv->pLog->LogError("CRenderer::CompressToDXT: Failed to compress texture to DXT: %dB allocated, %dB compressed", mipSize, size);
  }
}

bool CRenderer::DXTDecompress(byte *srcData,const size_t srcFileSize,byte *dstData,int nWidth,int nHeight,int nMips,ETEX_Format eSrcTF, bool bUseHW, int nDstBytesPerPix)
{
#if !defined(NULL_RENDERER) && !defined(PS3) && !defined(LINUX) && !defined(XENON)
  if (nDstBytesPerPix != 3 && nDstBytesPerPix != 4)
    return false;

	if (DDSFormats::IsNormalMap(eSrcTF))
	{
    assert(0);
		return false;
	}

  // NOTE: AMD64 port: implement
  if (!bUseHW)
  {
    CImageExtensionHelper::DDS_HEADER *ddsh;
    int blockSize = (eSrcTF == eTF_DXT1 || eSrcTF == eTF_CTX1) ? 8 : 16;
    int DXTSize = ((nWidth+3)/4)*((nHeight+3)/4)*blockSize;
    byte *dd = new byte [DXTSize + sizeof(CImageExtensionHelper::DDS_HEADER) + sizeof(DWORD)];

    DWORD dwMagic = MAKEFOURCC('D','D','S',' ');
    *(DWORD *)dd = dwMagic;
    ddsh = (CImageExtensionHelper::DDS_HEADER *)&dd[sizeof(DWORD)];
    memset(ddsh, 0, sizeof(CImageExtensionHelper::DDS_HEADER));
    cryMemcpy(&dd[sizeof(DWORD)+sizeof(CImageExtensionHelper::DDS_HEADER)], srcData, DXTSize);

    ddsh->dwSize = sizeof(CImageExtensionHelper::DDS_HEADER);
    ddsh->dwWidth = nWidth;
    ddsh->dwHeight = nHeight;
    ddsh->dwHeaderFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_LINEARSIZE;
    ddsh->dwPitchOrLinearSize = nWidth*nHeight*4/blockSize;
    if (eSrcTF == eTF_DXT1)
      ddsh->ddspf.dwFourCC = DDSFormats::DDSPF_DXT1.dwFourCC;
    else
    if (eSrcTF == eTF_DXT3)
      ddsh->ddspf.dwFourCC = DDSFormats::DDSPF_DXT3.dwFourCC;
    else
    if (eSrcTF == eTF_DXT5)
      ddsh->ddspf.dwFourCC = DDSFormats::DDSPF_DXT5.dwFourCC;
    ddsh->ddspf.dwSize = sizeof(ddsh->ddspf);
    ddsh->ddspf.dwFlags = DDS_FOURCC;
    ddsh->dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;

    sDDSData = dd;
//    int planes;
//    int lTotalWidth;
//    int rowBytes;
//    int width;
//    int height;
//    int src_format;

		//byte *_data = nvDXTdecompress(width, height, planes, lTotalWidth, rowBytes, src_format);

		nvImageContainer imageData;
		int readMIPMapCount=1;

		imageData.rgbaMIPImage.resize(1);
		imageData.rgbaMIPImage[0].realloc(nWidth,nHeight);		// to alloc memory in our DLL

 //Cut Diesel
/*
		if(nvDDS::nvDXTdecompress(imageData,PF_RGBA,readMIPMapCount,ReadDTXnFile,0)!=NV_OK)
		{
			delete []dd;
			return false;
		}
*/
 //Cut Diesel
		RGBAImage &mip = imageData.rgbaMIPImage[0];

		byte *_data = (byte *)mip.pixels();

		if (imageData.nPlanes != nDstBytesPerPix)
    {
      int n = imageData.width * imageData.height;
      if (imageData.nPlanes == 4)
      {
        assert (nDstBytesPerPix == 3);
        byte *data1 = _data;
        byte *dd1 = dstData;

        for (int i=0; i<n; i++)
        {
          dd1[0] = data1[2];
          dd1[1] = data1[1];
          dd1[2] = data1[0];
          dd1   += 3;
          data1 += 4;
        }
      }
      else
      if (imageData.nPlanes == 3)
      {
        assert (nDstBytesPerPix == 4);
        byte *data1 = _data;
        byte *dd1 = dstData;

        for (int i=0; i<n; i++)
        {
          dd1[0] = data1[2];
          dd1[1] = data1[1];
          dd1[2] = data1[0];
          dd1[3] = 255;
          dd1   += 4;
          data1 += 3;
        }
      }
    }
    else
      cryMemcpy(dstData, _data, imageData.width*imageData.height*imageData.nPlanes);

		delete []dd;
//		CRTDeleteArray(_data);
    return true;
  }
  else
#endif
  {
    return false;
  }
}

#if !defined(XENON) && !defined(PS3)

// wrapper used to remove DDS header from the NVDXT callback
static void *g_pNVDXT_UserData=0;
static MIPDXTcallback g_pNVDXT_Callback=0;
static size_t g_NVDXT_HeaderToRead = 4+sizeof(CImageExtensionHelper::DDS_HEADER);		// magic word+struct
///* //Diesel cut
static NV_ERROR_CODE NVIDIA_DXTWriteCallback(const void *buffer,size_t count, const MIPMapData * mipMapData, void * userData)
{
	assert(g_pNVDXT_UserData);
	assert(g_pNVDXT_Callback);

	uint8 *newbuffer=(uint8 *)buffer;
	size_t newcount=count;

	// ignore the first few bytes (dds header)
	if(newcount>g_NVDXT_HeaderToRead)
	{
		newbuffer+=g_NVDXT_HeaderToRead;
		newcount-=g_NVDXT_HeaderToRead;
		g_NVDXT_HeaderToRead=0;
	}
	else
	{
		g_NVDXT_HeaderToRead-=count;
		newbuffer+=count;
		newcount=0;
	}

	if(newcount)
		g_pNVDXT_Callback(newbuffer,newcount,userData);

	return NV_OK;
}
//*/ //Diesel cut
#endif

bool CRenderer::DXTCompress( byte *raw_data,int nWidth,int nHeight,ETEX_Format eTF, bool bUseHW, bool bGenMips,
	int nSrcBytesPerPix, const Vec3 vLumWeight, MIPDXTcallback callback)
{
#ifdef WIN32
	if(IsBadReadPtr(raw_data, nWidth*nHeight*nSrcBytesPerPix))
	{
		assert(0);
		iLog->Log("Warning: CRenderer::DXTCompress: invalid data passed to the function");
		return false;
	}
#endif

#if !defined(EXCLUDE_SQUISH_SDK)
  if(!bUseHW)
	if(CV_r_TextureCompressor==1)							// try squish
	if(bGenMips==false)				// todo: support mips
	{
		squish::SSquishOptions squishoptions;			assert(squishoptions.m_flags==0);
		if(vLumWeight.x>0)
		{
			squishoptions.m_Weights[0]=vLumWeight.x;
			squishoptions.m_Weights[1]=vLumWeight.y;
			squishoptions.m_Weights[2]=vLumWeight.z;
		}

		switch(eTF)
		{
			case eTF_DXT1:	squishoptions.m_flags = squish::kDxt1; break;
//			case eTF_DXT3:	squishoptions.m_flags = squish::kDxt3; break;		// not yet supported - would require proper alpha treatment
			case eTF_DXT5:	squishoptions.m_flags = squish::kDxt5; break;		// not very well tested yet
			default: assert(0);		// maybe squish would support the format
		}
		if(squishoptions.m_flags)
		{
			int size = squish::GetStorageRequirements(nWidth,nHeight,squishoptions.m_flags);

			uint8 *pOutMem = new uint8[size];

			if(pOutMem)
			{      
				//				uint32 nMips = CTexture::CalcNumMips(nWidth, nHeight);
				//				for(uint32 dwMip=0;dwMip<nMips;++dwMip)
				{
					//					if (!nWidth)
					//						nWidth = 1;
					//					if (!nHeight)
					//						nHeight = 1;

					//					int size = squish::GetStorageRequirements(nWidth,nHeight,squishformat);

					squish::CompressImageBGRX(raw_data,nWidth,nHeight,pOutMem,squishoptions);

//					MIPMapData MMData;
//					MMData.height	=	nHeight;
//					MMData.width	=	nWidth;
//					MMData.mipLevel	=	0;
//					(*callback)(pOutMem,size,&MMData,NULL);
					(*callback)(pOutMem,size,NULL);

						//					nWidth >>= 1;
						//					nHeight >>= 1;
				}

				delete [] pOutMem;
				return true;
			}

			return false;		// not enough memory
		}
	}
#endif // NULL_RENDERER

#if !defined(WIN64) && !defined(NULL_RENDERER) && !defined(PS3) && !defined(LINUX) && !defined(XENON)
	// NOTE: AMD64 port: implement
  if (!bUseHW)
  {
    nvCompressionOptions opt;

		if(vLumWeight.x>0)
		{
			opt.weight[0]=vLumWeight.x;
			opt.weight[1]=vLumWeight.y;
			opt.weight[2]=vLumWeight.z;
			opt.weightType=kUserDefinedWeighting;
		}

    switch(eTF)
    {
			case eTF_A4R4G4B4:
				opt.textureFormat = k4444;
				break;
			case eTF_A8R8G8B8:
        opt.textureFormat = k8888;
				break;
      case eTF_DXT1:
        opt.textureFormat = kDXT1;
    	  break;
      case eTF_DXT3:
        opt.textureFormat = kDXT3;
        break;
      case eTF_DXT5:
        opt.textureFormat = kDXT5;
        break;
			case eTF_A8:
				opt.textureFormat = kA8;
				break;
			case eTF_L8:
				opt.textureFormat = kL8;
				break;
      default:
        assert(0);
        return false;
    }
    opt.mipFilterType = kMipFilterQuadratic;
    if (bGenMips)
      opt.mipMapGeneration = kGenerateMipMaps;
    else
      opt.mipMapGeneration = kNoMipMaps;





		g_pNVDXT_UserData=0;							// not utilized yet
		g_pNVDXT_Callback=callback;
		g_NVDXT_HeaderToRead = 4+sizeof(CImageExtensionHelper::DDS_HEADER);		// magic word+struct

		nvDDS::nvDXTcompress(raw_data,nWidth,nHeight,nWidth*nSrcBytesPerPix,nvBGRA,&opt,NVIDIA_DXTWriteCallback); //Diesel ??? (Code\CryEngine\RenderDLL\Common\Textures\Image\NVDXT)

		g_pNVDXT_UserData=0;
		g_pNVDXT_Callback=0;
		g_NVDXT_HeaderToRead=0;


  }
  else
#endif
#if !defined(XENON) && !defined(PS3)
  {
    int i;
    byte *nDst = raw_data;
    if (nSrcBytesPerPix >= 3)
    {
      nDst = new byte[nWidth*nHeight*4];
			bool bUseAlpha = nSrcBytesPerPix > 3;
      if(bUseAlpha)
      {
        assert(nSrcBytesPerPix == 4);
        memcpy(nDst, raw_data, nWidth*nHeight*4);
      }
      else
      {
        for (i=0; i<nWidth*nHeight; i++)
        {
          nDst[i*4+0] = raw_data[i*nSrcBytesPerPix+0];
          nDst[i*4+1] = raw_data[i*nSrcBytesPerPix+1];
          nDst[i*4+2] = raw_data[i*nSrcBytesPerPix+2];
          nDst[i*4+3] = 255;
        }
      }
    }
    int nMips = 1;
    if (bGenMips)
      nMips = CTexture::CalcNumMips(nWidth, nHeight);
    int DXTSize = 0;
    byte *data = CTexture::Convert(nDst, nWidth, nHeight, nMips, eTF_A8R8G8B8, eTF, nMips, DXTSize, true);
    if (callback)
    {
      int blockSize = (eTF == eTF_DXT1 || eTF == eTF_CTX1) ? 8 : 16;
      int nOffs = 0;
      int wdt = nWidth;
      int hgt = nHeight;
      for (i=0; i<nMips; i++)
      {
        if (!wdt)
          wdt = 1;
        if (!hgt)
          hgt = 1;
        int nSize = ((wdt+3)/4)*((hgt+3)/4)*blockSize;
        assert(nSize+nOffs <= DXTSize);
//				MIPMapData MMData;
//				MMData.mipLevel	=	i;
//				MMData.width	=	wdt;
//				MMData.height	=	hgt;
//        (*callback)(&data[nOffs], nSize, &MMData, NULL);
				(*callback)(&data[nOffs], nSize, NULL);
        nOffs += nSize;
        wdt >>= 1;
        hgt >>= 1;
      }
    }
    delete [] data;
    if (nDst != raw_data)
      delete [] nDst;
  }
#endif
  return true;
}


bool CRenderer::WriteJPG(byte *dat, int wdt, int hgt, char *name, int src_bits_per_pixel, int nQuality )
{
#ifndef WIN64
	return ::WriteJPG(dat, wdt, hgt, name, src_bits_per_pixel, nQuality);
#else
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////
// IShaderPublicParams implementation class.
//////////////////////////////////////////////////////////////////////////

struct CShaderPublicParams : public IShaderPublicParams
{
  CShaderPublicParams()
  {
    m_nRefCount = 0;
  }

  virtual void AddRef() { m_nRefCount++; };
  virtual void Release() { if (--m_nRefCount <=0) delete this; };

  VIRTUAL void SetParamCount( int nParam ) { m_shaderParams.resize(nParam); }
  VIRTUAL int  GetParamCount() const { return m_shaderParams.size(); };

  VIRTUAL SShaderParam& GetParam( int nIndex )
  {
    assert( nIndex >= 0 && nIndex < (int)m_shaderParams.size()  );
    return m_shaderParams[nIndex];
  }

  VIRTUAL const SShaderParam& GetParam( int nIndex ) const
  {
    assert( nIndex >= 0 && nIndex < (int)m_shaderParams.size()  );
    return m_shaderParams[nIndex];
  }

  VIRTUAL void SetParam( int nIndex,const SShaderParam &param )
  {
    assert( nIndex >= 0 && nIndex < (int)m_shaderParams.size()  );
    m_shaderParams[nIndex] = param;
  }

  VIRTUAL void AddParam( const SShaderParam &param )
  {
    // shouldn't add existing parameter ?
    m_shaderParams.push_back(param);
  }

  VIRTUAL void SetParam(const char *pszName, UParamVal &pParam, EParamType nType = eType_FLOAT)
  {  
    int32 i;

    for (i=0; i<m_shaderParams.size(); i++)
    {
      if (!stricmp(pszName, m_shaderParams[i].m_Name))
      {
        break;
      }
    }

    if (i == m_shaderParams.size())
    {
      SShaderParam pr;
      strncpy(pr.m_Name, pszName, 32);    
      pr.m_Type = nType;
      m_shaderParams.push_back(pr);
    }

    SShaderParam::SetParam(pszName, &m_shaderParams, pParam);  
  }  

  VIRTUAL void SetShaderParams( const DynArray<SShaderParam> &pParams)
  {
    m_shaderParams = pParams;
  }

  /*virtual void AssignToRenderParams( struct SRendParams &rParams )
  {
    if (!m_shaderParams.empty())
      rParams.pShaderParams = &m_shaderParams;
  }*/

  VIRTUAL DynArray<SShaderParam> *GetShaderParams()
  {
    if (m_shaderParams.empty())
    {
      return 0;
    }

    return &m_shaderParams;
  }

  VIRTUAL const DynArray<SShaderParam> *GetShaderParams() const
  {
    if (m_shaderParams.empty())
    {
      return 0;
    }

    return &m_shaderParams;
  }

private:
  int m_nRefCount;
  DynArray<SShaderParam> m_shaderParams;
};

//////////////////////////////////////////////////////////////////////////
IShaderPublicParams* CRenderer::CreateShaderPublicParams()
{
	return new CShaderPublicParams;
}

#ifndef EXCLUDE_SCALEFORM_SDK

//////////////////////////////////////////////////////////////////////////
void CRenderer::SF_ConfigMask(ESFMaskOp maskOp, unsigned int stencilRef)
{
	int stencilFunc(FSS_STENCFUNC_ALWAYS);
	int stencilPass(FSS_STENCOP_KEEP);

	switch (maskOp)
	{
	case BeginSubmitMask_Clear:
		{
			stencilFunc = FSS_STENCFUNC_ALWAYS;
			stencilPass = FSS_STENCOP_REPLACE;
			break;
		}
	case BeginSubmitMask_Inc:
		{
			stencilFunc = FSS_STENCFUNC_EQUAL;
			stencilPass = FSS_STENCOP_INCR;
			break;
		}
	case BeginSubmitMask_Dec:
		{
			stencilFunc = FSS_STENCFUNC_EQUAL;
			stencilPass = FSS_STENCOP_DECR;
			break;
		}
	case EndSubmitMask:
		{
			stencilFunc = FSS_STENCFUNC_EQUAL;
			stencilPass = FSS_STENCOP_KEEP;
			break;
		}
	case DisableMask:
		{
			stencilFunc = FSS_STENCFUNC_ALWAYS;
			stencilPass = FSS_STENCOP_KEEP;
			break;
		}
	}

#ifdef DIRECT3D10
	EF_SetStencilState(STENC_FUNC(stencilFunc) | STENC_CCW_FUNC(stencilFunc) |
		STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_CCW_FAIL(FSS_STENCOP_KEEP) |
		STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_CCW_ZFAIL(FSS_STENCOP_KEEP) | 
		STENCOP_PASS(stencilPass) | STENCOP_CCW_PASS(stencilPass), stencilRef, 0xFFFFFFFF, 0xFFFFFFFF);
#else
	EF_SetStencilState(STENC_FUNC(stencilFunc) | STENCOP_FAIL(FSS_STENCOP_KEEP) | 
		STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(stencilPass), stencilRef, 0xFFFFFFFF, 0xFFFFFFFF);
#endif
}

//////////////////////////////////////////////////////////////////////////
int CRenderer::SF_CreateTexture(int width, int height, int numMips, unsigned char* pData, ETEX_Format eTF)
{
	char name[128];
	sprintf(name, "$SF_%d", m_TexGenID++);

	int flags(FT_DONT_STREAM | FT_DONT_RESIZE |  FT_DONT_RESIZE);
	flags |= !numMips ? FT_FORCE_MIPS : 0;

	CTexture* pTexture(CTexture::Create2DTexture(name, width, height, numMips, flags, pData, eTF, eTF));
	return (pTexture != 0) ? pTexture->GetID() : -1;
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::SF_GetMeshMaxSize(int& numVertices, int& numIndices) const
{
	numVertices = m_RP.m_MaxVerts;
	numIndices = m_RP.m_MaxTris * 3;
}

#endif // #ifndef EXCLUDE_SCALEFORM_SDK

//////////////////////////////////////////////////////////////////////////
uint16 CRenderer::PushFogVolumeContribution( const ColorF& fogVolumeContrib )
{
  int nThreadID = m_pRT->GetThreadList();

	const size_t maxElems( ( 1 << ( sizeof( uint16 ) * 8 ) ) - 1 );
	size_t numElems( m_RP.m_fogVolumeContibutions[nThreadID].size() );
	assert( numElems < maxElems);
	if( numElems >= maxElems )
		return (uint16) -1;

	m_RP.m_fogVolumeContibutions[nThreadID].push_back( fogVolumeContrib );
	return (uint16) ( m_RP.m_fogVolumeContibutions[nThreadID].size() - 1 );
}

//////////////////////////////////////////////////////////////////////////
const ColorF& CRenderer::GetFogVolumeContribution( uint16 idx ) const
{
	//assert( idx < m_RP.m_fogVolumeContibutions[m_RP.m_nProcessThreadID].size() );
	if( idx >= m_RP.m_fogVolumeContibutions[m_RP.m_nProcessThreadID].size() )
	{
		static ColorF s_defContrib( 0, 0, 0, 1 );
		return s_defContrib;
	}
	return m_RP.m_fogVolumeContibutions[m_RP.m_nProcessThreadID][idx];
}


const char * CRenderer::GetTextureFormatName(ETEX_Format eTF)
{
	return CTexture::NameForTextureFormat(eTF);
}

int CRenderer::GetTextureFormatDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF)
{
	return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, eTF);
}


//////////////////////////////////////////////////////////////////////////
ERenderType CRenderer::GetRenderType() const
{
#if defined(DIRECT3D10)



		return eRT_DX11;

#elif defined(DIRECT3D9)



		return eRT_DX9;

#elif defined(NULL_RENDERER)
	return eRT_Null;
#else
	return eRT_Undefined;
#endif
}
static bool GetDDSImageHeader(const char* filename,CImageExtensionHelper::DDS_HEADER* output)
{
  FILE* pRawFile = gEnv->pCryPak->FOpen(filename,"rb");
  if( !pRawFile )
    return false;
  gEnv->pCryPak->FSeek (pRawFile, sizeof(DWORD), SEEK_SET);
  gEnv->pCryPak->FReadRaw (output, 1, sizeof(CImageExtensionHelper::DDS_HEADER), pRawFile);
  gEnv->pCryPak->FClose (pRawFile);
  return true;
}

bool CRenderer::GetImageCaps(const char* filename,int& width,int& height) const
{
  string fileExt = PathUtil::GetExt(filename);
  if( stricmp(fileExt.c_str(),"dds")==0 )
  {
    CImageExtensionHelper::DDS_HEADER header;
    if( GetDDSImageHeader(filename,&header) )
    {
      width = header.dwWidth;
      height = header.dwHeight;
      return true;
    }
    else
    {
      return false;
    }
  }
  CImageFile* pImage = CImageFile::mfLoad_file(filename,false,FIM_ALPHA);
  width = pImage->mfGet_width();
  height = pImage->mfGet_height();
  if( pImage )
  {
    delete pImage;
    return true;
  }
  return false;
}

static int getBlockSize(const CImageExtensionHelper::DDS_PIXELFORMAT& format)
{
  if( format.dwFourCC==DDSFormats::DDSPF_DXT1.dwFourCC )
  {
    return (64/8);
  }
  else if( format.dwFourCC==DDSFormats::DDSPF_DXT5.dwFourCC )
  {
    return (64/8+64/8);
  }
  else if( format.dwFourCC==DDSFormats::DDSPF_3DC.dwFourCC )
  {
    return (128/8);
  }
  return 0;
}

inline int AlignTo4Nearest(float value)
{
  int iv = (int)value/4;
  int iv_div = (int)value%4;
  if( iv_div>2 )
  {
    iv=iv*4+4;
  }
  else
  {
    iv=iv*4;
  }
  return iv;
}

inline int AlignTo4Right(float value)
{
  //return AlignTo4Nearest(value);
  int iv = (int)(value/4.0f);
  iv*=4;
  if( value>0 )
  {
    float mod = fmod(value,4.0f);
    if( mod>0.0f )
      iv+=4;
    return iv;
  }
  return iv;
}

inline int AlignTo4Left(float value)
{
  //return AlignTo4Nearest(value);
  int iv = (int)(value/4.0f);
  iv*=4;
  if( value<0 )
  {
    float mod = fmod(value,4.0f);
    if( mod>0.0f )
      iv-=4;
    return iv;
  }
  return iv;
}

struct DDSProcessor
{
	bool loadData(const char* filename,CImageExtensionHelper::DDS_PIXELFORMAT& ioutputFormat) 
  {
    //
    outputFormat = ioutputFormat;
    //
    FILE* pRawFile = gEnv->pCryPak->FOpen(filename,"rb");
    if( !pRawFile )
      return false;
    gEnv->pCryPak->FSeek (pRawFile, 0, SEEK_END);
    long nFileSize = gEnv->pCryPak->FTell(pRawFile);
    gEnv->pCryPak->FSeek (pRawFile, 0, SEEK_SET);
    //
    m_fullbuffer.resize(nFileSize);
    //
    gEnv->pCryPak->FReadRaw (&m_fullbuffer[0], 1, nFileSize, pRawFile);
    gEnv->pCryPak->FClose (pRawFile);
    //
    //
    CImageExtensionHelper::DDS_HEADER* pDesc = (CImageExtensionHelper::DDS_HEADER*)(&m_fullbuffer[0]+sizeof(DWORD));
    input_Desc = *pDesc;
    m_srcData = (&m_fullbuffer[0]+sizeof(DWORD)+sizeof(CImageExtensionHelper::DDS_HEADER));
    //
    isDX1toDX1 = false;
    isDX1toDX5 = false;
    isDX5toDX5 = false;
    is3DCto3DC = false;
    //
    int srcBlockSize = getBlockSize(pDesc->ddspf);
    if( outputFormat.dwFourCC==DDSFormats::DDSPF_DXT1.dwFourCC && pDesc->ddspf.dwFourCC==DDSFormats::DDSPF_DXT1.dwFourCC ) 
    {
      isDX1toDX1 = true;
    }
    else if( outputFormat.dwFourCC==DDSFormats::DDSPF_DXT5.dwFourCC && pDesc->ddspf.dwFourCC==DDSFormats::DDSPF_DXT1.dwFourCC )
    {
      isDX1toDX5 = true;
    }
    else if( outputFormat.dwFourCC==DDSFormats::DDSPF_DXT5.dwFourCC && pDesc->ddspf.dwFourCC==DDSFormats::DDSPF_DXT5.dwFourCC )
    {
      isDX5toDX5 = true;
    }
    else if( outputFormat.dwFourCC==DDSFormats::DDSPF_3DC.dwFourCC && pDesc->ddspf.dwFourCC==DDSFormats::DDSPF_3DC.dwFourCC )
    {
      is3DCto3DC = true;
    }
    //
    return true;
  }
  //
  unsigned char* getMipData(byte* data,int dstMip,int fullW,int fullH,CImageExtensionHelper::DDS_PIXELFORMAT pixFormat);
  int getPitch(int dstMip,int fullW,int fullH,CImageExtensionHelper::DDS_PIXELFORMAT pixFormat);
  int getSrcMip(int dst_w,int dst_h,int src_w,int src_h,int src_w_mip0,int src_h_mip0);
  void BlitMipBlock(int dstMip,int dstTexWidth,int dstTexHeight,int dest_offsetx,int dest_offsety,int dest_x2,int dest_y2,int src_offsetx,int src_offsety,int src_x2,int src_y2,int src_w_mip0,int src_h_mip0,byte* pOutBuffer);
  //
  byte* m_srcData;
  std::vector<byte> m_fullbuffer;
  bool isDX1toDX1;
  bool isDX1toDX5;
  bool isDX5toDX5;
  bool is3DCto3DC;
  CImageExtensionHelper::DDS_HEADER input_Desc;
  CImageExtensionHelper::DDS_PIXELFORMAT outputFormat;
};


inline int Clamp(int val,int min_val,int max_val)
{
  if( val<min_val )
    return min_val;
  else if( val>max_val )
    return max_val;
  return val;
}

unsigned char* DDSProcessor::getMipData(byte* data,int dstMip,int fullW,int fullH,CImageExtensionHelper::DDS_PIXELFORMAT pixFormat)
{
  int o_width = fullW;
  int o_height = fullH;
  //
  int outBlockSize = getBlockSize(pixFormat);
  for(int n=0;n<dstMip;n++)
  {
    if( o_width<4 )
    {
      o_width = 4;
    }
    if( o_height<4 )
    {
      o_height = 4;
    }
    //
    int dstPitch4x4 = outBlockSize*o_width/4;
    data += dstPitch4x4*(o_height/4);
    //
    o_width >>=1;
    o_height >>=1;
    //
  }
  return data;
}

int DDSProcessor::getPitch(int dstMip,int fullW,int fullH,CImageExtensionHelper::DDS_PIXELFORMAT pixFormat)
{
  int o_width = fullW;
  int o_height = fullH;
  //
  int outBlockSize = getBlockSize(pixFormat);
  int dstPitch4x4 = outBlockSize*o_width/4;
  for(int n=0;n<dstMip;n++)
  {
    //
    o_width >>=1;
    o_height >>=1;
    //
    if( o_width<4 )
    {
      o_width = 4;
    }
    if( o_height<4 )
    {
      o_height = 4;
    }
    //
    dstPitch4x4 = outBlockSize*o_width/4;
  }
  return dstPitch4x4;
}

int DDSProcessor::getSrcMip(int dst_w,int dst_h,int src_w,int src_h,int src_w_mip0,int src_h_mip0)
{
  // 1 find 
  for(int n=0;!(src_w_mip0<4 && src_h_mip0<4);n++)
  {
    if( src_w_mip0<4 )
    {
      src_w_mip0 = 4;
    }
    if( src_h_mip0<4 )
    {
      src_h_mip0 = 4;
    }
    //
    if( src_w_mip0<=dst_w && src_h_mip0<=dst_h )
    {
      return n;
    }
    //
    src_w_mip0 >>=1;
    src_h_mip0 >>=1;
  }
  return 0;
}

void copyDXT5Transparency(byte* destBlockData,byte* srcBlockData,int destX,int destY,int srcX,int srcY )
{
  memcpy(destBlockData,srcBlockData,16/8);
  // transparency block 4x4 by 3bpp 
  uint64 alphaBlockData = *((uint64*)srcBlockData);
  alphaBlockData = alphaBlockData & 0xffffffffffff0000LL;
  int shiftBitCount = ((srcY)*4*3+srcX*3)+16;
  byte alphaVal = (byte)(alphaBlockData>>shiftBitCount);
  alphaVal = alphaVal & 0x7;
  int dstshiftBitCount = ((destY)*4*3+(destX)*3)+16;
  uint64 alphaBlockRes = ((uint64)alphaVal)<<dstshiftBitCount;
  *((uint64*)destBlockData)|=alphaBlockRes;
}


void DDSProcessor::BlitMipBlock(int dstMip,int dstTexWidth,int dstTexHeight,int dest_offsetx,int dest_offsety,int dest_x2,int dest_y2,int src_offsetx,int src_offsety,int src_x2,int src_y2,int src_w_mip0,int src_h_mip0,byte* pOutBuffer)
{
  int dst_w = dest_x2-dest_offsetx;
  int dst_h = dest_y2-dest_offsety;
  int src_w = src_x2-src_offsetx;
  int src_h = src_y2-src_offsety;
  // find src mip level
  byte* pDstMipData = getMipData(pOutBuffer,dstMip,dstTexWidth,dstTexHeight,outputFormat);
  int srcMip = getSrcMip(dst_w,dst_h,src_w,src_h,src_w_mip0,src_h_mip0);
  byte* pSrcMipData = getMipData(m_srcData,srcMip,input_Desc.dwWidth,input_Desc.dwHeight,input_Desc.ddspf);
  //
  int srcPitch4x4 = getPitch(srcMip,input_Desc.dwWidth,input_Desc.dwHeight,input_Desc.ddspf);
  int srcBlockSize = getBlockSize(input_Desc.ddspf);
  //
  int dstPitch4x4 = getPitch(dstMip,dstTexWidth,dstTexHeight,outputFormat);
  int dstBlockSize = getBlockSize(outputFormat);
  //
  if( dst_w<src_w )
  {
    int factor = src_w/dst_w;
    src_w = min(dst_w,src_w);
    src_offsetx = src_offsetx/factor;
  }
  if( dst_h<src_h )
  {
    int factor = src_h/dst_h;
    src_h = min(dst_h,src_h);
    src_offsety = src_offsetx/factor;
  }
  int resizeFactorX =  dst_w/src_w;
  int resizeFactorY =  dst_h/src_h;
  //
  // to match
  bool bResize = false;
  if( resizeFactorX!=1 || resizeFactorY!=1 )
  {
    bResize = true;
  }
  //
  if( !bResize )
  {
    int y_src=src_offsety;
    for(int y_dst=dest_offsety;y_dst<dest_y2;y_dst+=4,y_src+=4)
    {
      int x_src=src_offsetx;
      for(int x_dst=dest_offsetx;x_dst<dest_x2;x_dst+=4,x_src+=4)
      {
        byte* srcBlockDataLine = pSrcMipData+(y_src/4)*srcPitch4x4;
        byte* dstBlockDataLine = pDstMipData+(y_dst/4)*dstPitch4x4;
        // error if xCur and yCur is not bound 4x4 block
        if( isDX1toDX1 || isDX5toDX5 || is3DCto3DC )
        {
          // full line in one pass
          byte* srcBlockData = srcBlockDataLine+(x_src/4)*srcBlockSize;
          byte* dstBlockData = dstBlockDataLine+(x_dst/4)*dstBlockSize;
          memcpy(dstBlockData,srcBlockData,src_w/4*srcBlockSize);
          break;
        }
        else if( isDX1toDX5 )
        {
          byte* srcBlockData = srcBlockDataLine+(x_src/4)*srcBlockSize;
          byte* dstBlockData = dstBlockDataLine+(x_dst/4)*dstBlockSize;
          // no transparency
          // copy color block
          memcpy(dstBlockData+64/8,srcBlockData,srcBlockSize);
          // fill 1 alphas
          memset(dstBlockData,0xffffff,64/8);
        }
        else
        {
          assert(0);
        }
      }
    }
  }
  else
  {
    std::vector<byte> srcBlock; 
    //
    srcBlock.resize(srcBlockSize);
    //
    int exremeDataSize = (8*4)/8;
    int exremeDataOffset = (6*4*4)/8;
    int colorDataOffset = 0;
    //
    for(int y_dst=dest_offsety;y_dst<dest_y2;y_dst+=4)
    {
      for(int x_dst=dest_offsetx;x_dst<dest_x2;x_dst+=4)
      {
        // clear block
        memset(&srcBlock[0],0,srcBlock.size());
        //
        int curDstXDelta = x_dst-dest_offsetx;
        int curDstYDelta = y_dst-dest_offsety;
        int x_src=(src_offsetx+curDstXDelta)/resizeFactorX;
        int y_src=(src_offsety+curDstYDelta)/resizeFactorY;
        int x_src_div_block=(src_offsetx+curDstXDelta)%resizeFactorX;
        int y_src_div_block=(src_offsety+curDstYDelta)%resizeFactorY;
        //
        byte* srcBlockDataLine = pSrcMipData+(y_src/4)*srcPitch4x4;
        byte* dstBlockDataLine = pDstMipData+(y_dst/4)*dstPitch4x4;
        //
        byte* dstBlockData = dstBlockDataLine+(x_dst/4)*dstBlockSize;
        byte* srcBlockData = srcBlockDataLine+(x_src/4)*srcBlockSize;
        // copy extreme data
        if( input_Desc.ddspf.dwFourCC==DDSFormats::DDSPF_DXT1.dwFourCC )
        {
          memcpy(&srcBlock[0],srcBlockData,(16+16)/8);
        }
        else if( input_Desc.ddspf.dwFourCC==DDSFormats::DDSPF_DXT5.dwFourCC )
        {
          // color extr
          memcpy(&srcBlock[0]+64/8,srcBlockData+64/8,(16+16)/8);
        }
        else if( input_Desc.ddspf.dwFourCC==DDSFormats::DDSPF_3DC.dwFourCC )
        {
        }
        // copy color/alpha data
        for(int by=0;by<4;by++)
        {
          for(int bx=0;bx<4;bx++)
          {
            int x_src_block=(src_offsetx+curDstXDelta+bx)/resizeFactorX;
            int y_src_block=(src_offsety+curDstYDelta+by)/resizeFactorY;
            int x_src_div=x_src_block%4;
            int y_src_div=y_src_block%4;
            if( input_Desc.ddspf.dwFourCC==DDSFormats::DDSPF_DXT1.dwFourCC )
            {
              // one row is 1 byte 
              byte srcColor = srcBlockData[(16+16)/8+y_src_div*8/8];
              srcColor = ((0x3<<((x_src_div)*2)) & srcColor)>>((x_src_div)*2);
              srcBlock[by+(16+16)/8] |= srcColor<<((bx)*2);
            }
            else if( input_Desc.ddspf.dwFourCC==DDSFormats::DDSPF_DXT5.dwFourCC )
            {
              // one row is 1 byte 
              byte srcColor = srcBlockData[(16+16)/8+y_src_div*8/8+64/8];
              srcColor = ((0x3<<((x_src_div)*2)) & srcColor)>>((x_src_div)*2);
              srcBlock[by+(16+16)/8+64/8] |= srcColor<<((bx)*2);
              // transparency block 
              copyDXT5Transparency(&srcBlock[0],srcBlockData,bx,by,x_src_div,y_src_div);
            }
            else if( input_Desc.ddspf.dwFourCC==DDSFormats::DDSPF_3DC.dwFourCC )
            {
              copyDXT5Transparency(&srcBlock[0],srcBlockData,bx,by,x_src_div,y_src_div);
              copyDXT5Transparency(&srcBlock[0]+(8+8+3*16)/8,srcBlockData+(8+8+3*16)/8,bx,by,x_src_div,y_src_div);
            }
          }
        }
        srcBlockData = &srcBlock[0];
        // error if xCur and yCur is not bound 4x4 block
        if( isDX1toDX1 || isDX5toDX5 || is3DCto3DC )
        {
          // 
          memcpy(dstBlockData,srcBlockData,srcBlockSize);
        }
        else if( isDX1toDX5 )
        {
          // no transparency
          // copy color block
          memcpy(dstBlockData+64/8,srcBlockData,srcBlockSize);
          // fill 1 alphas
          memset(dstBlockData,0xffffff,64/8);
        }
        else
        {
          assert(0);
        }
      }
    }
  }
}

bool CRenderer::MergeImages(const char* output_filename,int out_width,int out_height,const char** files,int* offsetsX,int* offsetsY,int* widths,int* heights,int* src_offsetsX,int* src_offsetsY,int* src_width,int* src_height, int count) const
{
  //SaveTga()
  //
  int numOutMips = 0;
  CImageExtensionHelper::DDS_PIXELFORMAT outputFormat;
  for(int n=0;n<count;n++)
  {
    if( offsetsX[n]%4!=0 )
    {
      CryLog( "texture %s x offset %d in atlas is not divisible by 4",files[n],offsetsX[n]);
      return false;
    }
    if( offsetsY[n]%4!=0 )
    {
      CryLog( "texture %s y offset %d in atlas is not divisible by 4",files[n],offsetsY[n]);
      return false;
    }
    if( src_offsetsX[n]%4!=0 )
    {
      CryLog( "src texture %s x offset %d in atlas is not divisible by 4",files[n],offsetsX[n]);
      return false;
    }
    if( src_offsetsY[n]%4!=0 )
    {
      CryLog( "src texture %s y offset %d in atlas is not divisible by 4",files[n],offsetsY[n]);
      return false;
    }
    if( widths[n]%4!=0 )
    {
      CryLog( "texture %s width %d in atlas is not divisible by 4",files[n],widths[n]);
      return false;
    }
    if( heights[n]%4!=0 )
    {
      CryLog( "texture %s height %d in atlas is not divisible by 4",files[n],heights[n]);
      return false;
    }

    CImageExtensionHelper::DDS_HEADER header;
    GetDDSImageHeader(files[n],&header);
    if( n==0)
    {
      outputFormat = header.ddspf;
    }
    if( header.ddspf.dwFourCC==DDSFormats::DDSPF_DXT1.dwFourCC )
    {
      if( outputFormat.dwFourCC == DDSFormats::DDSPF_DXT5.dwFourCC )
      {
        CryLog("texmap %s is dxt1 in series of dxt5 textures",files[n]);
      }
      if( outputFormat.dwFourCC == DDSFormats::DDSPF_DXT1.dwFourCC )
      {
        // dxt5 overrides dxtc1
        outputFormat = DDSFormats::DDSPF_DXT1;
      }
    }
    else if( header.ddspf.dwFourCC==DDSFormats::DDSPF_DXT5.dwFourCC )
    {
      if( outputFormat.dwFourCC == DDSFormats::DDSPF_DXT1.dwFourCC )
      {
        CryLog("texmap %s is dxt5 in series of dxt1 textures",files[n]);
      }
      outputFormat = DDSFormats::DDSPF_DXT5;
    }
    else if( header.ddspf.dwFourCC==DDSFormats::DDSPF_3DC.dwFourCC )
    {
      if( outputFormat.dwFourCC!=DDSFormats::DDSPF_3DC.dwFourCC )
      {
        CryLog( "texture %s in not in 3dc in series of 3dc textures",files[n]);
        // all should be in 3dc
        return false;
      }
      outputFormat = DDSFormats::DDSPF_3DC;
    }
    else
    {
      CryLog( "unsupported to merge textures with formats %04X and %04X. Texture:%s",header.ddspf.dwFourCC,outputFormat.dwFourCC,files[n]);
      // unsupported
      return false;
    }
    //    int OutMips = max(header.dwMipMapCount,numOutMips);
    //  int clampedOutMips = (32-NumLeadingZeros())-2;
    numOutMips = max((int)header.dwMipMapCount,numOutMips);
  }
  //
  /*
  int w = out_width;
  int h = out_height;
  numOutMips = 1;
  while(!(w==1 && h==1))
  {
    w>>=1;
    h>>=1;
    if( w==0 )
      w = 1;
    if( h==0 )
      h = 1;
    //
    numOutMips++;
  }
  //
  numOutMips = 2;
  */
  //
  //numOutMips = 1;
  //
  int numPixelsTotal = 0;
  int w = out_width;
  int h = out_height;
  for(int m=0;m<numOutMips;m++)
  {
    if( w<4 )
    {
      w = 4;
    }
    if( h<4 )
    {
      h = 4;
    }
    numPixelsTotal+=w*h;
    //
    w>>=1;
    h>>=1;
  }
  //
  int outBlockSize = 0;
  outBlockSize = getBlockSize(outputFormat);
  // 64 bits for 4x4 block - DXT1
  // 64 bits color and 64 bits alpha 4x4 block DXT5
  // 128 bits for 4x4 block 3DC
  std::vector<byte> data(numPixelsTotal/16*outBlockSize,0);
  DDSProcessor processor;
  //
  std::set<string> uniq_files;
  for(int n=0;n<count;n++)
  {
    uniq_files.insert(files[n]);
  }
  //
  for( std::set<string>::iterator it = uniq_files.begin();it!=uniq_files.end();++it)
  {
    processor.loadData(*it,outputFormat);
    //
    for(int n=0;n<count;n++)
    {
      // dispatch only *it src texture
      if( strcmp(files[n],*it)!=0 )
        continue;
      //
      // ignore duplicates
      bool isDuplicate = false;
      /*
      for(int n1=0;n1<n;n1++)
      {
      if( strcmp(files[n1],files[n])==0 )
      {
      isDuplicate = true;
      break;
      }
      }
      if( isDuplicate )
      {
      continue;
      }
      */
      //
      //
      // till 4x4 block
      int i_rect_width = src_width[n];
      int i_rect_height = src_height[n];
      int i_offsetX = src_offsetsX[n];
      int i_offsetY = src_offsetsY[n];
      int i_fullWidth = processor.input_Desc.dwWidth;
      int i_fullHeight = processor.input_Desc.dwHeight;
      //
      byte* curOMipData = &data[0];
      byte* endOutData = &data[data.size()];
      int o_rect_width = widths[n];
      int o_rect_height = heights[n];
      int o_offsetx = offsetsX[n];
      int o_offsety = offsetsY[n];
      int o_fullWidth = out_width;
      int o_fullHeight = out_height;
      //numOutMips = 2;
      byte* last4x4Block = 0;
      for(int m=0;m<numOutMips;m++)
      {
        if( o_fullWidth<4 )
        {
          o_fullWidth = 4;
        }
        if( o_fullHeight<4 )
        {
          o_fullHeight = 4;
        }
        if( i_fullWidth<4 )
        {
          i_fullWidth = 4;
        }
        if( i_fullHeight<4 )
        {
          i_fullHeight = 4;
        }
        // in any case by 4x4 block
        // DEST calc offset block
        int dest_offsetx = Clamp(AlignTo4Left((float)o_offsetx),0,o_fullWidth);
        int dest_offsety = Clamp(AlignTo4Left((float)o_offsety),0,o_fullHeight);
        int dest_x2 = Clamp(AlignTo4Right((float)(o_offsetx+o_rect_width)),0,o_fullWidth);
        int dest_y2 = Clamp(AlignTo4Right((float)(o_offsety+o_rect_height)),0,o_fullHeight);
        // SRC calc offset block
        int src_offsetx = Clamp(AlignTo4Left((float)i_offsetX),0,i_fullWidth);
        int src_offsety = Clamp(AlignTo4Left((float)i_offsetY),0,i_fullHeight);
        int src_x2 = Clamp(AlignTo4Right((float)(i_offsetX+i_rect_width)),0,i_fullWidth);
        int src_y2 = Clamp(AlignTo4Right((float)(i_offsetY+i_rect_height)),0,i_fullHeight);
        //
        processor.BlitMipBlock(m,out_width,out_height,dest_offsetx,dest_offsety,dest_x2,dest_y2,src_offsetx,src_offsety,src_x2,src_y2,src_width[n],src_height[n],&data[0]);
        //
        o_fullWidth >>=1;
        o_fullHeight >>=1;
        o_offsetx >>=1;
        o_offsety >>=1;
        o_rect_width >>=1;
        o_rect_height >>=1;
        //
        if( i_rect_width>o_rect_width || i_rect_height>o_rect_height )
        {
          i_offsetX >>=1;
          i_offsetY >>=1;
          i_rect_width >>=1;
          i_rect_height >>=1;
          i_fullWidth >>=1;
          i_fullHeight >>=1;
          if( i_rect_width<4 )
          {
            i_rect_width = 4;
          }
          if( i_rect_height<4 )
          {
            i_rect_height = 4;
          }
        }
        if( o_rect_width<4 && o_rect_height<4 )
        {
          break;
        }
      }
    }
  }
  //
  CImageExtensionHelper::DDS_HEADER ddsh;
  ZeroStruct(ddsh);
  ddsh.dwSize = sizeof(CImageExtensionHelper::DDS_HEADER);
  ddsh.dwWidth	= out_width;
  ddsh.dwHeight = out_height;
  ddsh.dwMipMapCount = numOutMips>1 ? numOutMips : 0;
  ddsh.ddspf = outputFormat;
  ddsh.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
  ddsh.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
  //
  FILE* pRawFile = gEnv->pCryPak->FOpen(output_filename,"wb");
  if( !pRawFile )
    return false;
  //
  DWORD dwMagic = MAKEFOURCC('D','D','S',' ');
  gEnv->pCryPak->FWrite(&dwMagic,sizeof(dwMagic),1,pRawFile);
  gEnv->pCryPak->FWrite(&ddsh,sizeof(ddsh),1,pRawFile);
  gEnv->pCryPak->FWrite(&data[0],data.size(),1,pRawFile);
  gEnv->pCryPak->FClose(pRawFile);
  //
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void SDrawCallCountInfo::Update( CRenderObject *pObj )
{
  SRenderPipeline& RESTRICT_REFERENCE rRP = gRenDev->m_RP;
  if( ((IRenderNode *)pObj->m_pRenderNode) )
  {
    pPos = ((IRenderNode *)pObj->m_pRenderNode)->GetPos();

    if( rRP.m_nBatchFilter & (FB_DETAIL|FB_GLOW|FB_SCATTER|FB_MOTIONBLUR|FB_REFRACTIVE|FB_CAUSTICS|FB_CUSTOM_RENDER|FB_LAYER_EFFECT|FB_RAIN|FB_FUR|FB_DEBUG) )
      nMisc++;
    else
      if( !(rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN) )
      {
        if( rRP.m_nBatchFilter & FB_GENERAL )
          nGeneral++;
        else
          if( rRP.m_nBatchFilter & FB_Z )
            nZpass++;
      }
      else
        nShadows++;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void S3DEngineCommon::Update()
{
	I3DEngine *p3DEngine = gEnv->p3DEngine;

  // Camera vis area
  IVisArea *pCamVisArea =  p3DEngine->GetVisAreaFromPos( gRenDev->GetRCamera().Orig );
  m_pCamVisAreaInfo.nFlags &= ~VAF_MASK;
  if( pCamVisArea )
  {
    m_pCamVisAreaInfo.nFlags |= VAF_EXISTS_FOR_POSITION;
    if( pCamVisArea->IsConnectedToOutdoor() )
      m_pCamVisAreaInfo.nFlags |= VAF_CONNECTED_TO_OUTDOOR;
    if( pCamVisArea->IsAffectedByOutLights() )
      m_pCamVisAreaInfo.nFlags |= VAF_AFFECTED_BY_OUT_LIGHTS;
  }

	// Update ocean info
	m_OceanInfo.m_fWaterLevel = p3DEngine->GetWaterLevel();
	m_OceanInfo.m_nOceanRenderFlags = p3DEngine->GetOceanRenderFlags();
	m_OceanInfo.m_vCausticsParams = gEnv->p3DEngine->GetCausticsParams();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
namespace WaterVolumeStaticData { void GetMemoryUsage( ICrySizer *pSizer ); }

void CRenderer::GetMemoryUsage( ICrySizer *pSizer )
{
	// should be called by all derived classes

	pSizer->AddObject( m_TextMessages[0] );
	pSizer->AddObject( m_TextMessages[1] );
	pSizer->AddObject( m_RP );
	pSizer->AddObject( m_pRT );
	pSizer->AddObject(m_DevBufMan);
	pSizer->AddObject(CRenderObject::m_sFreePermObjData);

	WaterVolumeStaticData::GetMemoryUsage(pSizer);
}


float CRenderer::GetGPUFrameTime()
{















	float fGPUidle = m_fTimeGPUIdlePercent[m_RP.m_nProcessThreadID] * 0.01f; // normalise %
	float fGPUload = 1.0f - fGPUidle; // normalised non-idle time
	float fGPUtime = (iTimer->GetFrameTime() * fGPUload); //GPU time in seconds
	return fGPUtime;

}

#if defined(ENABLE_GPU_TIMERS)
//
//GPU Timers
//
GpuTimerEvent						GpuTimerEvent::s_events[s_numBuffers][s_maxNumEvents];
uint32									GpuTimerEvent::s_eventCount[s_numBuffers] = {0};
bool										GpuTimerEvent::s_outOfEvents[s_numBuffers] = {0};

GpuTimerEvent*					GpuTimerEvent::s_currentOpenEvent = NULL;

uint32									GpuTimerEvent::s_readIdx = 0;
uint32									GpuTimerEvent::s_callbackIdx = 1;
uint32									GpuTimerEvent::s_writeIdx = 1;

void CRenderer::RT_BeginGPUTimer( const char* name )
{
	int writeEventCount = GpuTimerEvent::s_eventCount[GpuTimerEvent::s_writeIdx];

	if( writeEventCount>=GpuTimerEvent::s_maxNumEvents )
	{
		GpuTimerEvent::s_outOfEvents[GpuTimerEvent::s_writeIdx] = true;
		return;
	}

	GpuTimerEvent *writeEvents = GpuTimerEvent::s_events[GpuTimerEvent::s_writeIdx];
	
	GpuTimerEvent *newEvent = &writeEvents[writeEventCount];
	
	//initialise new event
	newEvent->Init(name);

	//event is now a child of the current open event (if there is one)
	newEvent->parent = GpuTimerEvent::s_currentOpenEvent;
	
	//new event becomes current open event
	GpuTimerEvent::s_currentOpenEvent = newEvent;

	RT_InsertGpuCallback(writeEventCount, GpuTimerEvent::BeginTimerCallback);

	GpuTimerEvent::s_eventCount[GpuTimerEvent::s_writeIdx]++;
}

void CRenderer::RT_EndGPUTimer(const char* name )
{
	if( GpuTimerEvent::s_outOfEvents[GpuTimerEvent::s_writeIdx] )
	{
		return;
	}

	if(GpuTimerEvent::s_currentOpenEvent==NULL || GpuTimerEvent::s_currentOpenEvent->open==false)
	{
		//logic error
		assert(0);
		return;
	}
	
	GpuTimerEvent* writeEvents = GpuTimerEvent::s_events[GpuTimerEvent::s_writeIdx];

	GpuTimerEvent::s_currentOpenEvent->open = false;
	
	//pointer arithmetic to determine index
	int index = ( (int)GpuTimerEvent::s_currentOpenEvent - (int)writeEvents ) / sizeof(GpuTimerEvent);
	assert(GpuTimerEvent::s_currentOpenEvent == &writeEvents[index]); //sanity check

	RT_InsertGpuCallback(index, GpuTimerEvent::EndTimerCallback);

	//parent becomes current open event
	GpuTimerEvent::s_currentOpenEvent = GpuTimerEvent::s_currentOpenEvent->parent;
}

void CRenderer::SwapGpuTimers()
{
	assert(m_pRT->IsRenderThread());
	
	//increment write index now
	GpuTimerEvent::s_writeIdx = (GpuTimerEvent::s_writeIdx + 1) % GpuTimerEvent::s_numBuffers;

	GpuTimerEvent::s_eventCount[GpuTimerEvent::s_writeIdx] = 0;
	GpuTimerEvent::s_outOfEvents[GpuTimerEvent::s_writeIdx] = false;

	for(int i=0; i<GpuTimerEvent::s_maxNumEvents; i++)
	{
		GpuTimerEvent::s_events[GpuTimerEvent::s_writeIdx][i].Reset();
	}

	GpuTimerEvent::s_currentOpenEvent = NULL;
	
	//increment gpu callback index when gpu reaches this command
	RT_InsertGpuCallback(0, GpuTimerEvent::SwapTimersCallback);
}

void CRenderer::RenderGpuStats()
{
	assert(m_pRT->IsRenderThread());

	const int startX = 45;
	const int startY = 30;

	if( GpuTimerEvent::s_outOfEvents[GpuTimerEvent::s_readIdx] )
	{
			WriteXY(startX,startY, 2.f,2.f, 1,0,0,1, "RUN OUT OF GPU TIMER EVENTS");
			return;
	}
	
	uint32 numEvents = GpuTimerEvent::s_eventCount[GpuTimerEvent::s_readIdx];
	
	if(numEvents>0)
	{
		GpuTimerEvent* events = GpuTimerEvent::s_events[GpuTimerEvent::s_readIdx];

		const int colWidth = 10;
		int rowHeight = 9;
		float textScale = 0.9f;

		int yPos = startY;

		GpuTimerEvent* currentParent = NULL;

		LARGE_INTEGER ticksPerSecond;
    QueryPerformanceFrequency( &ticksPerSecond );
    double ticksPerMillisecond = (double)ticksPerSecond.QuadPart * 0.001;
		
		WriteXY( startX, yPos, 1.5f,1.5f, 1,1,0,1, "GPU Timers" );
		yPos+=rowHeight*2;

		for(int i=0; i<numEvents; i++)
		{
			GpuTimerEvent* parentEvent = events[i].parent;
			int xPos = startX;

			if(parentEvent)
			{
				while(parentEvent)
				{
					xPos+=colWidth;
					parentEvent = parentEvent->parent;
				}
			}
			else
			{
				yPos+=rowHeight;
			}

			if(events[i].totalTime > 0)
			{
				if(strncmp(events[i].name, "Debug Node", 10)==0)
				{
					continue;
					//WriteXY(xPos,yPos, 1.f,1.f, 1,0.5,0,1,"%s:", events[i].name.c_str() );
					//WriteXY(315,yPos, 1.f,1.f, 1,0.5,0,1,"%.4fms", events[i].totalTime / ticksPerMillisecond );
				}
				else
				{
          float fTime = (float)events[i].totalTime / (float)ticksPerMillisecond;
          ColorF col = (fTime > 1.f) ? Col_Red : Col_Green;
          WriteXY(xPos,yPos, textScale, textScale, col.r,col.g,col.b,1, "%s:", events[i].name );
          WriteXY(315,yPos, textScale, textScale, col.r,col.g,col.b,1, "%.3f ms", fTime );
				}
			}
			else
			{
				WriteXY(xPos,yPos, 1.f,1.f,1,0,0,1,"%s, CALLBACK NOT RECIEVED", events[i].name);
			}
			yPos += rowHeight;
		}
	}
}


void CRenderer::RenderGpuStatsDebugNode()
{
	assert(m_pRT->IsRenderThread());

	const int xPos = 605;
	const int yPos = 260;

	if( GpuTimerEvent::s_outOfEvents[GpuTimerEvent::s_readIdx] )
	{
			WriteXY(xPos, yPos, 1.f,1.f, 1,0,0,1, "GPU Time: Out Of Events");
			return;
	}
	
	uint32 numEvents = GpuTimerEvent::s_eventCount[GpuTimerEvent::s_readIdx];
	
	if(numEvents>0)
	{
		GpuTimerEvent* events = GpuTimerEvent::s_events[GpuTimerEvent::s_readIdx];

		GpuTimerEvent* currentParent = NULL;

		LARGE_INTEGER ticksPerSecond;
    QueryPerformanceFrequency( &ticksPerSecond );
    double ticksPerMillisecond = (double)ticksPerSecond.QuadPart * 0.001;

		double totalTime = 0.0;
			
		for(int i=0; i<numEvents; i++)
		{
			if(events[i].totalTime > 0)
			{
				if(strncmp(events[i].name, "Debug Node", 10)==0)
				{
					totalTime += events[i].totalTime;
				}
			}
		}

		WriteXY(xPos, yPos, 1.f,1.f, 1,0.5,0,1, "GPU Time: %.4fms", totalTime/ticksPerMillisecond);
	}
}

//static
void GpuTimerEvent::BeginTimerCallback(DWORD idx)
{



}

//static
void GpuTimerEvent::EndTimerCallback(DWORD idx)
{




}

//static
void GpuTimerEvent::SwapTimersCallback(DWORD idx)
{
	GpuTimerEvent::s_readIdx = GpuTimerEvent::s_callbackIdx;
	GpuTimerEvent::s_callbackIdx = (GpuTimerEvent::s_callbackIdx + 1) % GpuTimerEvent::s_numBuffers;
}

#endif

void CRenderer::GetRenderTimes( SRenderTimes &outTimes )
{
	//Query render times on main thread
	outTimes.fWaitForMain = m_fTimeWaitForMain[m_RP.m_nFillThreadID];
	outTimes.fWaitForRender = m_fTimeWaitForRender[m_RP.m_nFillThreadID];
	outTimes.fWaitForGPU = m_fTimeWaitForGPU[m_RP.m_nFillThreadID]; 
	outTimes.fTimeProcessedRT = m_fTimeProcessedRT[m_RP.m_nFillThreadID];
	outTimes.fTimeGPUIdlePercent = m_fTimeGPUIdlePercent[m_RP.m_nFillThreadID];
}



//Diesel new
//enum NV_ERROR_CODE nvDDS::nvDXTdecompress(class nvImageContainer &,enum nvPixelFormat,int,enum NV_ERROR_CODE (__cdecl*)(void *,unsigned __int64,void *),void *)
//{
//return 0;
//}
//Diesel new



#include UNIQUE_VIRTUAL_WRAPPER(IShaderPublicParams)
