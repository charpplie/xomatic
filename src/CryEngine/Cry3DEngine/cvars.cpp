//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   cvars.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: console variables used in 3dengine
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "3dEngine.h"
#include "IRenderer.h"

//////////////////////////////////////////////////////////////////////////
void OnTimeOfDayVarChange( ICVar *pArgs )
{
	gEnv->p3DEngine->GetTimeOfDay()->SetTime( Cry3DEngineBase::GetCVars()->e_TimeOfDay );
}


void OnTimeOfDaySpeedVarChange( ICVar *pArgs )
{
	ITimeOfDay::SAdvancedInfo advInfo;
	gEnv->p3DEngine->GetTimeOfDay()->GetAdvancedInfo(advInfo);
	advInfo.fAnimSpeed = Cry3DEngineBase::GetCVars()->e_TimeOfDaySpeed;
	gEnv->p3DEngine->GetTimeOfDay()->SetAdvancedInfo(advInfo);
}

void CVars::Init()
{
  REGISTER_CVAR(e_Fog, 1, VF_CHEAT, 
		"Activates global height/distance based fog");
	REGISTER_CVAR(e_FogVolumes, 1, VF_CHEAT, 
		"Activates local height/distance based fog volumes");
	REGISTER_CVAR(e_Entities, 1, VF_CHEAT, 
		"Activates drawing of entities and brushes");
	REGISTER_CVAR(e_SkyBox, 1, VF_CHEAT, 
		"Activates drawing of skybox and moving cloud layers");
	REGISTER_CVAR(e_WaterOcean, 1, VF_CHEAT, 
		"Activates drawing of ocean");
  REGISTER_CVAR(e_WaterOceanBottom, 1, VF_CHEAT, 
		"Activates drawing bottom of ocean");
  
  REGISTER_CVAR(e_WaterOceanFFT, 0, VF_NULL, 
		"Activates fft based ocean");

  REGISTER_CVAR(e_WaterOceanSoftParticles, 1, VF_NULL, 
		"Enables ocean soft particles");
  
  REGISTER_CVAR(e_DebugDrawShowOnlyCompound, 0, VF_NULL, 
    "e_DebugDraw shows only Compound (less efficient) static meshes");

	REGISTER_CVAR(e_DebugDraw, 0, VF_CHEAT,
		"Draw helpers with information for each object (same number negative hides the text)\n"
		" 1: Name of the used cgf, polycount, used LOD\n"
		" 2: Color coded polygon count\n"
		" 3: Show color coded LODs count, flashing color indicates no Lod\n"
		" 4: Display object texture memory usage\n"
		" 5: Display color coded number of render materials\n"
		" 6: Display ambient color\n"
		" 7: Display tri count, number of render materials, texture memory\n"
		" 8: RenderWorld statistics (with view cones)\n"
		" 9: RenderWorld statistics (with view cones without lights)\n"
		"10: Render geometry with simple lines and triangles\n"
		"11: Render occlusion geometry additionally\n"
		"12: Render occlusion geometry without render geometry\n"
		"13: Display occlusion amount (used during AO computations)\n"
		"15: Display helpers\n"
		"17: Streaming info (buffer sizes)\n"
		"18: Streaming info (required streaming speed)\n");

	REGISTER_CVAR(e_DetailObjects, 1, VF_NULL, 
		"Turn detail objects on/off");
	REGISTER_CVAR(e_TerrainDetailMaterials, 1, VF_CHEAT, 
		"Activates drawing of detail materials on terrain ground");
	REGISTER_CVAR(e_TerrainDetailMaterialsDebug, 0, VF_CHEAT,
		"Shows number of materials in use per terrain sector");
	REGISTER_CVAR(e_TerrainDetailMaterialsViewDistZ, 128.f, VF_NULL, 
		"Max view distance of terrain Z materials");
	REGISTER_CVAR(e_TerrainDetailMaterialsViewDistXY, 2048.f, VF_NULL, 
		"Max view distance of terrain XY materials");
	REGISTER_CVAR(e_SunClipPlaneRange, 256.f, VF_NULL, 
		"Maximum possible clip plane range for sun light frustum in meters");
  REGISTER_CVAR(e_SunAngleSnapSec, 0.1f, VF_NULL, 
		"Sun dir snap control");
  REGISTER_CVAR(e_SunAngleSnapDot, 0.999999f, VF_NULL, 
		"Sun dir snap control");
	
	REGISTER_CVAR(e_Particles, 1, VF_CHEAT, 
		"Activates drawing of particles");
	REGISTER_CVAR(e_ParticlesDebug, 0, VF_CHEAT | VF_BITFIELD, 
		"Particle debug flags: flags+ enables, flags- disables\n"
		" 1 = show basic stats\n"
		" f+ = show fill rate stats\n"
		" r+ = show reuse/reject stats\n"
		" b+ = draw bounding boxes and labels\n"
		" s+ = log emitter and particle counts by effect (for 1 frame)\n"
		" e+ = log particle system timing info (for 1 frame)\n"
		" d+ = log particle system timing info (for new slowest frames)\n"
		" t+ = log particle texture usage\n"
		" c+ = disable clipping against water and vis area bounds\n"
		" x+ = disable DX11 geom shader\n"
		" z+ = freeze particle system");
	REGISTER_CVAR(e_ParticlesThread, 1, VF_BITFIELD, 
		"Enable particle threading");




	REGISTER_CVAR(e_ParticlesObjectCollisions, 1, VF_NULL, 
		"Enable particle/object collisions for SimpleCollision:\n"
		"  1 = against static objects only, 2 = dynamic also");
	REGISTER_CVAR(e_ParticlesPreload, 1, VF_NULL, 
		"Enable preloading of all particle effects at the beginning");
	REGISTER_CVAR(e_ParticlesLod, 1, VF_NULL, 
		"Multiplier to particle count");
	REGISTER_CVAR(e_ParticlesMinDrawPixels, 1, VF_NULL, 
		"Pixel size cutoff for rendering particles -- fade out earlier");
	REGISTER_CVAR(e_ParticlesMaxScreenFill, 64, VF_NULL, 
		"Max screen-fulls of particles to draw");

	REGISTER_CVAR(e_Roads, 1, VF_CHEAT, 
		"Activates drawing of road objects");

  REGISTER_CVAR(e_Decals, 1, VF_NULL, 
    "Activates drawing of decals (game decals and hand-placed)");
  REGISTER_CVAR(e_DecalsMaxTrisInObject, 15000, VF_NULL, 
    "Do not create decals on objects having more than X triangles");
  REGISTER_CVAR(e_DecalsAllowGameDecals,	1, VF_NULL, 
		"Allows creation of decals by game (like weapon bullets marks)");
  REGISTER_CVAR(e_DecalsHitCache, 1, VF_CHEAT, 
		"Use smart hit caching for bullet hits (may cause no decals in some cases)");
	REGISTER_CVAR(e_DecalsMerge, 1, VF_NULL, 
		"Combine pieces of decals into one render call");
	REGISTER_CVAR(e_DecalsPreCreate, 1, VF_NULL, 
		"Pre-create decals at load time");
	REGISTER_CVAR(e_DecalsScissor, 1, VF_NULL,
		"Enable decal rendering optimization by using scissor");
	REGISTER_CVAR(e_DecalsClip, 1, VF_NULL, 
		"Clip decal geometry by decal bbox");
	REGISTER_CVAR(e_DecalsRange, 20.0f, VF_NULL, 
		"Less precision for decals outside this range");
	REGISTER_CVAR(e_DecalsWrapAroundMinSize, 0.05f, VF_NULL, 
		"Controls usage of wrapping around objects feature.\n"
		"When decal radius is smaller than X - simple quad decal is used");
	REGISTER_CVAR(e_DecalsLifeTimeScale, 1.f, VF_NULL, 
		"Allows to increase or reduce decals life time for different specs");
	REGISTER_CVAR(e_DecalsNeighborMaxLifeTime, 4.f, VF_CHEAT, 
		"If not zero - new decals will force old decals to fade in X seconds");
  REGISTER_CVAR(e_DecalsOverlapping, 0, VF_CHEAT, 
		"If zero - new decals will not be spawned if the distance to nearest decals less than X");

	REGISTER_CVAR(e_VegetationBending, 2, VF_NULL, 
		"Debug");
	REGISTER_CVAR(e_VegetationAlphaBlend, 1, VF_CHEAT, 
		"Allow alpha blending for vegetations");
	REGISTER_CVAR(e_VegetationAlphaBlendNear, 0.0f, VF_CHEAT, 
		"Allow alpha blending nearby vegetation");	
  REGISTER_CVAR(e_VegetationUseTerrainColor, 1, VF_NULL, 
		"Allow blend with terrain color for vegetations");
  REGISTER_CVAR(e_VegetationSphericalSkinning,	1, VF_CHEAT, 
		"Activates vegetation spherical skinning support");  
  REGISTER_CVAR(e_VegetationWind, 1, VF_CHEAT, 
		"Activates vegetation with wind support");
  REGISTER_CVAR(e_Vegetation, 1, VF_CHEAT, 
		"Activates drawing of distributed objects like trees");
  REGISTER_CVAR(e_VegetationSprites, 1, VF_CHEAT, 
		"Activates drawing of sprites instead of distributed objects at far distance");
	REGISTER_CVAR(e_VegetationBoneInfo, 0, VF_CHEAT, 
		"Logs information about number of bones in each vegetation object loaded");
  REGISTER_CVAR(e_VegetationAlignToTerrainAmount, 1.f, VF_CHEAT, 
    "0 - no align, 1 - full align");
	REGISTER_CVAR(e_VegetationSpritesMinDistance, 8.0f, VF_NULL, 
		"Sets minimal distance when distributed object can be replaced with sprite");

	REGISTER_CVAR(e_VegetationSpritesDistanceRatio, 1.0f, VF_NULL, 
		"Allows changing distance on what vegetation switch into sprite");
  REGISTER_CVAR(e_VegetationSpritesDistanceCustomRatioMin, 1.0f, VF_NULL, 
		"Clamps SpriteDistRatio setting in vegetation properties");
  REGISTER_CVAR(e_ForceDetailLevelForScreenRes, 0, VF_NULL, 
		"Force sprite distance and other values used for some specific screen resolution, 0 means current");
	REGISTER_CVAR(e_VegetationMinSize,0.f,VF_REQUIRE_LEVEL_RELOAD,
		"Minimal size of static object, smaller objects will be not rendered");

	REGISTER_CVAR(e_Wind, 1, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_WindAreas, 1, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_Shadows, 1, VF_NULL, 
		"Activates drawing of shadows");
  REGISTER_CVAR(e_ShadowsLodBiasFixed, 0, VF_NULL, 
    "Simplifies mesh for shadow map generation by X LOD levels");
  REGISTER_CVAR(e_ShadowsLodBiasInvis, 1, VF_NULL, 
    "Simplifies mesh for shadow map generation by X LOD levels, if object is not visible in main frame");
  REGISTER_CVAR(e_ShadowsFromTerrain, 1, VF_NULL, 
		"Enable shadows from terrain");
  REGISTER_CVAR(e_ShadowsOnAlphaBlend, 1, VF_NULL, 
		"Enable shadows on alphablended ");
	REGISTER_CVAR(e_ShadowsFrustums, 0, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_ShadowsDebug, 0, VF_CHEAT, 
		"0=off, 2=visualize shadow maps on the screen");
  REGISTER_CVAR(e_ShadowsClouds, 1, VF_NULL, 
		"Cloud shadows");	// no cheat var because this feature shouldn't be strong enough to affect gameplay a lot
	REGISTER_CVAR(e_ShadowsMaxTexRes,1024, VF_NULL, 
		"Set maximum resolution of shadow map\n256(faster), 512(medium), 1024(better quality)");
	REGISTER_CVAR(e_ShadowsSlopeBias, 4.0, VF_NULL, 
		"Shadows slope bias for shadowgen");
	REGISTER_CVAR(e_ShadowsOnWater, 0, VF_NULL, 
		"Enable/disable shadows on water");

  REGISTER_CVAR(e_ShadowsMasksLimit, 0, VF_NULL,
		"Maximum amount of allocated shadow mask textures\n"
		"This limits the number of shadow casting lights overlapping\n"
		"0=disable limit(unpredictable memory requirements)\n"
		"1=one texture (4 channels for 4 lights)\n"
		"2=two textures (8 channels for 8 lights), ...");

	REGISTER_CVAR(e_ShadowsCastViewDistRatio, 0.8f, VF_NULL, 
		"View dist ratio for shadow maps");
	REGISTER_CVAR(e_ShadowsOcclusionCulling, 0, VF_NULL, 
		"Enable/disable shadow-caster test against the occlusion buffer");
	REGISTER_CVAR(e_ShadowsOcclusionCullingCaster, 0, VF_NULL, 
		"Clips the caster extrusion to the zero height.");
	REGISTER_CVAR(e_GsmRange, 30.0f, VF_NULL, 
		"Size of LOD 0 GSM area (in meters)");
	REGISTER_CVAR(e_GsmRangeStep, 3.0f, VF_NULL, 
		"Range of next GSM lod is previous range multiplied by step");
	REGISTER_CVAR(e_GsmLodsNum, 5, VF_NULL, 
		"Number of GSM lods (0..5)");
	REGISTER_CVAR(e_GsmDepthBoundsDebug, 0, VF_NULL, 
		"Debug GSM bounds regions calculation");
	REGISTER_CVAR(e_GsmStats, 0, VF_CHEAT, 
		"Show GSM statistics 0=off, 1=enable debug to the screens");
	REGISTER_CVAR(e_GsmViewSpace, 0, VF_NULL, 
		"0=world axis aligned GSM layout, 1=Rotate GSM frustums depending on view camera");
  REGISTER_CVAR(e_GsmCache, 0, VF_NULL, 
		"Cache sun shadows maps over several frames 0=off, 1=on if MultiGPU is deactivated");
  REGISTER_CVAR(e_GsmCacheLodOffset, 3, VF_NULL, 
		"Makes first X GSM lods not cached");
	REGISTER_CVAR(e_GsmRangeStepTerrain, 16, VF_NULL, 
		"gsm_range_step for last gsm lod containing terrain");
  REGISTER_CVAR(e_GsmScatterLodDist, 70, VF_NULL, 
		"Size of Scattering LOD GSM in meters");

	REGISTER_CVAR(e_DebugMask, 0, VF_CHEAT, 
		"debug variable to activate certain features for debugging (each bit represents one feature\nbit 0(1): use EFSLIST_TERRAINLAYER\nbit 1(2):\nbit 3(4):\nbit 4(8):");
	
	REGISTER_CVAR(e_Terrain, 1, VF_CHEAT, 
		"Activates drawing of terrain ground");
  REGISTER_CVAR(e_TerrainDeformations, 0, VF_CHEAT,
		"Allows in-game terrain surface deformations");
  REGISTER_CVAR(e_LevelAutoPrecacheCameraJumpDist, 0, VF_CHEAT, 
    "When not 0 - Force full pre-cache of textures, procedural vegetation and shaders\n"
    "if camera moved for more than X meters in one frame or on new cut scene start");
  REGISTER_CVAR(e_LevelAutoPrecacheTerrainAndProcVeget, 1, VF_CHEAT,
		"Force auto pre-cache of terrain textures and procedural vegetation");
  REGISTER_CVAR(e_LevelAutoPrecacheTexturesAndShaders, 1, VF_CHEAT, 
    "Force auto pre-cache of general textures and shaders");
  REGISTER_CVAR(e_LevelAutoPrecacheCgf, 1, VF_CHEAT, 
    "Force auto pre-cache of CGF render meshes");

	REGISTER_CVAR(e_TerrainBBoxes, 0, VF_CHEAT, 
		"Show terrain nodes bboxes");
  REGISTER_CVAR(e_TerrainNormalMap, 1, VF_NULL, 
		"Use terrain normal map for base pass if available");
	REGISTER_CVAR(e_TerrainOcclusionCulling, 1, VF_CHEAT, 
		"heightmap occlusion culling with time coherency 0=off, 1=on");
  REGISTER_CVAR(e_TerrainOcclusionCullingStepSizeDelta, 1.05f, VF_CHEAT, 
		"Step size scale on every next step (for version 1)");
  REGISTER_CVAR(e_TerrainOcclusionCullingMaxDist,	200.f, VF_NULL, 
		"Max length of ray (for version 1)");
  REGISTER_CVAR(e_TerrainOcclusionCullingVersion, 1, VF_CHEAT, 
		"0 - old, 1 - new");
  REGISTER_CVAR(e_TerrainOcclusionCullingDebug, 0, VF_CHEAT, 
		"Draw sphere on every terrain height sample");
	REGISTER_CVAR(e_TerrainOcclusionCullingMaxSteps, 50, VF_CHEAT, 
		"Max number of tests per ray (for version 0)");
	REGISTER_CVAR(e_TerrainOcclusionCullingStepSize, 4, VF_CHEAT, 
		"Initial size of single step (in heightmap units)");
  REGISTER_CVAR(e_TerrainTextureDebug, 0, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_TerrainTextureStreamingDebug, 0, VF_CHEAT, 
		"Debug");
  REGISTER_CVAR(e_TerrainTextureStreamingPoolItemsNum, 64, VF_REQUIRE_LEVEL_RELOAD, 
    "Specifies number of textures in terrain base texture streaming pool");
  REGISTER_CVAR(e_TerrainLog, 0, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_TerrainDrawThisSectorOnly, 0, VF_CHEAT, 
		"1 - render only sector where camera is and objects registered in sector 00\n"
		"2 - render only sector where camera is");
	REGISTER_CVAR(e_TerrainOcclusionCullingPrecision, 0.25f, VF_CHEAT, 
		"Density of rays");
  REGISTER_CVAR(e_TerrainOcclusionCullingPrecisionDistRatio, 3.f, VF_CHEAT, 
		"Controls density of rays depending on distance to the object");
	REGISTER_CVAR(e_TerrainLodRatio, 1.f, VF_NULL, 
		"Set heightmap LOD");

  REGISTER_CVAR(e_OcclusionCullingViewDistRatio, 0.5f, VF_CHEAT, 
		"Skip per object occlusion test for very far objects - culling on tree level will handle it");

  REGISTER_CVAR(e_TerrainTextureLodRatio, 1.0f, VF_NULL, 
		"Adjust terrain base texture resolution on distance");
  
	REGISTER_CVAR(e_Sun, 1, VF_CHEAT, 
		"Activates sun light source");
  REGISTER_CVAR(e_CoverageBuffer, 1, VF_CHEAT, 
		"Activates usage of software coverage buffer.\n"
		"1 - camera culling only\n"
		"2 - camera culling and light-to-object check");
  REGISTER_CVAR(e_CoverageBufferCullIndividualBrushesMaxNodeSize, 0, VF_CHEAT, 
    "128 - cull only nodes of scene tree and very big brushes\n"
		"0 - cull all brushes individually");
  REGISTER_CVAR(e_CoverageBufferTerrain, 0, VF_NULL, 
		"Activates usage of coverage buffer for terrain");
  REGISTER_CVAR(e_CoverageBufferTerrainLodShift, 2, VF_NULL, 
		"Controls tessellation of terrain mesh");
  REGISTER_CVAR(e_CoverageBufferTerrainMaxDistance, 512.f, VF_NULL, 
		"Only near sectors are rasterized");
  REGISTER_CVAR(e_CoverageBufferTerrainElevationShift, 4.f, VF_NULL, 
		"Shift low lods down in order to avoid false occlusion");
  REGISTER_CVAR(e_CoverageBufferTerrainLodRatio, 4.f, VF_NULL, 
		"Terrain lod ratio for mesh rendered into cbuffer");
	REGISTER_CVAR(e_CoverageBufferVersion, 2, VF_CHEAT, 
		"1 Vladimir's, 2MichaelK's");
	REGISTER_CVAR(e_CoverageBufferHW, 0, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferClipPlanesNum, 6, VF_CHEAT, 
		"Debug"); 
	REGISTER_CVAR(e_CoverageBufferDebug, 0, VF_CHEAT, 
		"Display content of main camera coverage buffer");
	REGISTER_CVAR(e_CoverageBufferDebugFreeze, 0, VF_CHEAT, 
		"Freezes view matrix/-frustum ");
	REGISTER_CVAR(e_CoverageBufferDrawOccluders, 0, VF_CHEAT, 
		"Debug draw of occluders for coverage buffer");
	REGISTER_CVAR(e_CoverageBufferTestMode, 2, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferBias, 1.05f, VF_CHEAT, 
		"Coverage buffer z-biasing");
	REGISTER_CVAR(e_CoverageBufferBiasAdjustment, 3000.f, VF_CHEAT, 
		"Coverage buffer dynamic z-biasing");
	REGISTER_CVAR(e_CoverageBufferRotationSafeCheck, 0, VF_CHEAT, 
		"Coverage buffer safe checking for rotation 0=disabled 1=enabled 2=enabled for out of frustum object");
	REGISTER_CVAR(e_CoverageBufferLightsDebugSide, -1, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferDebugDrawScale, 1, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferResolution, 128, VF_NULL, 
		"Resolution of software coverage buffer");
	REGISTER_CVAR(e_CoverageBufferAccurateOBBTest, 0, VF_NULL, 
		"Checking of OBB boxes instead of AABB or bounding rects");
	REGISTER_CVAR(e_CoverageBufferTolerance, 0, VF_NULL, 
		"amount of visible pixel that will still identify the object as covered");
	REGISTER_CVAR(e_CoverageBufferOccludersTestMinTrisNum, 0, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferOccludersViewDistRatio, 1.0f, VF_CHEAT, 
		"Debug");
  REGISTER_CVAR(e_CoverageBufferOccludersLodRatio, 0.25f, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferLazyTest, 1, VF_CHEAT, 
		"Do not test visible objects every frame");
	REGISTER_CVAR(e_CoverageBufferTreeDepth, 7, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferTreeDebug, 0, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_CoverageBufferMaxAddRenderMeshTime, 2, VF_CHEAT, 
		"Max time for unlimited AddRenderMesh");
	REGISTER_CVAR(e_CullerThread, 1, VF_CHEAT, 
		"The hw thread number on XENON that the culler will run on, only read at startup");
  
  REGISTER_CVAR(e_DynamicLightsMaxCount, 512, VF_CHEAT, 
    "Sets maximum amount of dynamic light sources");

  REGISTER_CVAR(e_DynamicLights, 1, VF_CHEAT, 
		"Activates dynamic light sources");
  REGISTER_CVAR(e_DynamicLightsFrameIdVisTest, 1, VF_NULL, 
		"Use based on last draw frame visibility test");
  REGISTER_CVAR(e_DynamicLightsConsistentSortOrder, 1, VF_NULL, 
		"Debug");

	REGISTER_CVAR(e_HwOcclusionCullingWater,	1, VF_NULL, 
		"Activates usage of HW occlusion test for ocean");
	REGISTER_CVAR(e_HwOcclusionCullingObjects,0, VF_CHEAT, 
		"Activates usage of HW occlusion test for objects");

  REGISTER_CVAR(e_Portals, 1, VF_CHEAT, 
		"Activates drawing of visareas content (indoors), values 2,3,4 used for debugging");
  REGISTER_CVAR(e_PortalsBigEntitiesFix, 1, VF_CHEAT, 
		"Enables special processing of big entities like vehicles intersecting portals");
	REGISTER_CVAR(e_DynamicLightsMaxEntityLights, 16, VF_NULL, 
		"Set maximum number of lights affecting object");
	REGISTER_CVAR(e_MaxViewDistance, -1.0, VF_CHEAT, 
		"Far clipping plane distance");
  REGISTER_CVAR(e_MaxViewDistSpecLerp, 1, VF_NULL, 
		"1 - use max view distance set by designer for very high spec\n0 - for very low spec\nValues between 0 and 1 - will lerp between high and low spec max view distances");
  REGISTER_CVAR(e_MaxViewDistFullDistCamHeight, 1000, VF_CHEAT, 
		"Debug");
	REGISTER_CVAR(e_WaterVolumes, 1, VF_CHEAT, 
		"Activates drawing of water volumes");
  REGISTER_CVAR(e_WaterWaves, 0, VF_CHEAT, 
		"Activates drawing of water waves");
  REGISTER_CVAR(e_WaterWavesTesselationAmount,	5, VF_NULL, 
		"Sets water waves tessellation amount");  
  REGISTER_CVAR(e_WaterTesselationAmount, 10, VF_NULL, 
		"Set tessellation amount");  
  REGISTER_CVAR(e_WaterTesselationSwathWidth, 12, VF_NULL, 
		"Set the swath width for the boustrophedonic mesh stripping"); 
	REGISTER_CVAR(e_BBoxes, 0, VF_CHEAT, 
		"Activates drawing of bounding boxes");

  REGISTER_CVAR(e_StreamCgfPoolSize, 24, VF_NULL, 
    "Render mesh cache size in MB");
  REGISTER_CVAR(e_StreamCgfMaxTasksInProgress, 4, VF_CHEAT, 
    "Maximum number of files simultaneously requested from streaming system");
	REGISTER_CVAR(e_StreamCgf, 0, VF_CHEAT, 
    "Enable streaming of static render meshes");
  REGISTER_CVAR(e_StreamCgfDebug, 0, VF_CHEAT, 
    "Draw helpers and other debug information about CGF streaming\n"
    " 1: Draw color coded boxes for objects taking more than e_StreamCgfDebugMinObjSize,\n"
		"    also shows are the LOD's stored in single CGF or were split into several CGF's\n"
    " 2: Trace into console every loading and unloading operation\n"
    " 3: Print list of currently active objects taking more than e_StreamCgfDebugMinObjSize KB");
  REGISTER_CVAR(e_StreamCgfDebugMinObjSize, 500, VF_CHEAT, 
    "Threshold for objects debugging in KB");
  REGISTER_CVAR(e_StreamCgfDebugHeatMap, 0, VF_CHEAT, 
    "Generate and show mesh streaming heat map\n"
    " 1: Generate heat map for entire level\n"
    " 2: Show last heat map");  
  REGISTER_CVAR(e_StreamCgfPredicitionDistance, 16.f, VF_CHEAT, 
    "Prediction distance for CGF render mesh streaming");
  REGISTER_CVAR(e_StreamCgfVisObjPriority, 0.5f, VF_CHEAT, 
    "Priority boost for visible objects\n"
    "0 - visible objects has no priority over invisible objects, camera direction does not affect streaming\n"
    "1 - visible objects has highest priority, in case of trashing will produce even more trashing");

	REGISTER_CVAR(e_ScissorDebug, 0, VF_CHEAT, 
		"Debug");

#ifdef SUPPORT_LM
	REGISTER_CVAR(e_ram_maps, 1,VF_REQUIRE_LEVEL_RELOAD, 
		"Use RAM (real-time ambient maps) on brushes");
	REGISTER_CVAR(e_ram_DumpDXF, 0, VF_CHEAT, 
		"Dump DXF files of the UV-set of ram objects/brushes to visualize overlapping UVs");
#endif // SUPPORT_LM

  REGISTER_CVAR(e_Brushes, 1, VF_CHEAT, 
		"Draw brushes");
  REGISTER_CVAR(e_CullLightsPerTriangleMinObjRadius, 8.f, VF_CHEAT, 
		"Debug");

//  REGISTER_CVAR(e_SceneMerging, 0, VF_CHEAT, 
	//	"Merge brushes on level loading");
//  REGISTER_CVAR(e_SceneMergingViewDistRatio, 3.f, VF_CHEAT, 
  //  "Controls size of rendered nodes depends from distance");
//  REGISTER_CVAR(e_SceneMergingMinNodeSize, 16.f, VF_CHEAT, 
  //  "Min size of merge area (smallest areas are used near camera)");
//  REGISTER_CVAR(e_SceneMergingMaxTrisPerInputChunk, 100, VF_CHEAT, 
  //  "Merge only chunks containing less than X triangles");
/*	REGISTER_CVAR(e_scene_merging_compact_vertices, 1, VF_NULL, 
		"Minimize number of vertices in decals and merged brushes");
	REGISTER_CVAR(e_scene_merging_show_onlymerged, 0, VF_CHEAT, 
		"Show only merged brushes");
	REGISTER_CVAR(e_scene_merging_max_tris_in_chunk, 64, VF_NULL, 
		"Only objects having less than X triangles per chunk will me merged");
	REGISTER_CVAR(e_scene_merging_max_time_ms, 5, VF_NULL, 
		"Spend only X ms per frame on merging");
	REGISTER_CVAR(e_scene_merging_min_merge_distance, 0, VF_NULL, 
		"Don't merge nodes closer than X");*/

	REGISTER_CVAR(e_OnDemandPhysics, gEnv->pSystem->IsEditor() ? 0:1, VF_NULL, 
		"Turns on on-demand vegetation physicalization");
	REGISTER_CVAR(e_OnDemandMaxSize, 20.0f, VF_NULL, 
		"Specifies the maximum size of vegetation objects that are physicalized on-demand");
	REGISTER_CVAR(e_Sleep, 0, VF_CHEAT, 
		"Sleep X in C3DEngine::Draw");
	REGISTER_CVAR(e_Objects, 1, VF_CHEAT, 
		"Render or not all objects");
	REGISTER_CVAR(e_Render, 1, VF_CHEAT, 
		"Enable engine rendering");
  REGISTER_CVAR(e_ObjectsTreeBBoxes, 0, VF_CHEAT, 
    "Debug draw of object tree bboxes");
/*  REGISTER_CVAR(e_obj_tree_min_node_size, 0, VF_CHEAT, 
		"Debug draw of object tree bboxes");
  REGISTER_CVAR(e_obj_tree_max_node_size, 0, VF_CHEAT, 
		"Debug draw of object tree bboxes");*/
  REGISTER_CVAR(e_MtTest, 0, VF_CHEAT, 
    "MT Debug");
  REGISTER_CVAR(e_MtTestBuffer, 0, VF_CHEAT, 
    "MT RenderRenderMesh Debug");
	REGISTER_CVAR(e_ObjStats, 0, VF_CHEAT, 
		"Show instances count");
  REGISTER_CVAR(e_ObjFastRegister, 1, VF_CHEAT, 
		"Debug");

	REGISTER_CVAR(e_OcclusionVolumes, 1, VF_CHEAT, 
		"Enable occlusion volumes(antiportals)");
	REGISTER_CVAR(e_OcclusionVolumesViewDistRatio, 0.05f, VF_NULL, 
		"Controls how far occlusion volumes starts to occlude objects");

	REGISTER_CVAR(e_PrecacheLevel, 0, VF_NULL, 
		"Pre-render objects right after level loading");
	REGISTER_CVAR(e_Dissolve, 0, VF_NULL, 
		"Objects alphatest_noise_fading out on distance");
	REGISTER_CVAR(e_Lods, 1, VF_NULL, 
		"Load and use LOD models for static geometry");
	REGISTER_CVAR(e_LodsForceUse, 1, VF_NULL,
		"Force using LODs even if triangle count do not suit");
  REGISTER_CVAR(e_LodDistShader, 16.0f, VF_NULL,
    "Set distance to enable shader lod ");
  

	REGISTER_CVAR(e_Voxel, 1, VF_NULL, 
		"Render voxels");
  REGISTER_CVAR(e_VoxTerTexRangeScale, 1.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerVegetationEx, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerPlanarProjection, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerMemWarning, 10, VF_NULL, 
    "Show message box when allocations reach X GB");
  REGISTER_CVAR(e_VoxTerTexFormat, eTF_DXT1, VF_NULL, 
    "Mega-texture format: 22=eTF_DXT1, 24=eTF_DXT5, 7=eTF_A4R4G4B4, 8=eTF_R5G6B5");
  REGISTER_CVAR(e_VoxTerMemoryUsageDebug, 0, VF_NULL, 
    "Show only segments where mesh and textures memory usage is higher than X kb");
  REGISTER_CVAR(e_VoxTerDiskUsageDebug, 0, VF_NULL, 
    "Show only segments where disk usage is higher than X kb");
  REGISTER_CVAR(e_VoxTerTexUsageDebug, 0, VF_NULL, 
    "Show only segments where texture memory usage is higher than X kb");
  REGISTER_CVAR(e_VoxTerMeshUsageDebug, 0, VF_NULL, 
    "Show only segments where mesh memory usage is higher than X kb");
  REGISTER_CVAR(e_VoxTerTexPoolSizeForEditor, 128, VF_NULL, 
    "Maximum amount of re-mesh textures cached in memory (in MB)");
  REGISTER_CVAR(e_VoxTerBorderLodPlus, 2, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerMaxMeshBuildDepth, 5, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerMaxMeshBuildDepthLimit, 5, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebug, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerRoadsCheck, 1, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerRoadsCullDist, 1.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerThreads, max(1,GetSystem()->GetLogicalCPUCount()-1), VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebugBuildLeafs, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerOptimizeLods, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebugAreaSizeMax, 128, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebugAreaSizeRatio, 160, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebugLodRatio, 35.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebugX, 0.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebugY, 0.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerDebugNodesGenDepth, 3, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerAddBrushes, 1, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerRelaxation, 0, VF_NULL, 
    "Use X vertex relaxation passes during voxel terrain mesh generation");
  REGISTER_CVAR(e_VoxTerMeshSize, 2.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTraceMessages, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTer, 0, VF_NULL, 
		"Debug");
  REGISTER_CVAR(e_VoxTerActivateAll, 0, VF_NULL, 
    "Actuate all terrain meshes and textures available marked as needed for game play");
  REGISTER_CVAR(e_VoxTerHeightmapEditing, 0, VF_NULL | VF_REQUIRE_LEVEL_RELOAD, 
    "Debug");
  REGISTER_CVAR(e_VoxTerHeightmapEditingCustomLayerInfo, 0, VF_NULL | VF_REQUIRE_LEVEL_RELOAD, 
    "Debug");
  REGISTER_CVAR(e_VoxTerShadows, 1, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerLog, 1, VF_NULL, 
    "Debug");
	REGISTER_CVAR(e_VoxTerOnTheFlyIntegration, 0, VF_NULL, 
		"Debug");
  REGISTER_CVAR(e_VoxTerIntegrateVegetations, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerInGameTextureStreaming, 1, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerUpdateTextures, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerAsyncMeshCompiling, 1, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerAsyncPhysicalization, 1, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTexDebug, 0, VF_NULL, 
		"Debug");
  REGISTER_CVAR(e_VoxTerHideIntegrated, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTexBuildOnCPU, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTexBuildsPerFrame, 256, VF_NULL, 
    "Debug"); 
  REGISTER_CVAR(e_VoxTerMaxMeshLods, 9, VF_NULL, 
    "Debug"); 
  REGISTER_CVAR(e_VoxTerMaxMeshesInMem, 2000, VF_NULL, 
    "Debug"); 
  REGISTER_CVAR(e_VoxTerTexTiling, 1.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTexResInternal, 256, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTexResFinal, 256, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTexResBumpDownscale, 1, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerTexResExportDownscale, 1, VF_NULL, 
    "values 1 or 2 will downscale textures during exporting by factor of 2 or 4");
  REGISTER_CVAR(e_VoxTerTexRebuildEveryFrame, 0, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerMixMask, 0, VF_NULL, 
    "Store blend info, base color and normals in textures");
  REGISTER_CVAR(e_VoxTerOverlayBlend, 0, VF_NULL, 
    "Use overlay blending mode");
  REGISTER_CVAR(e_VoxTerViewDistRatio, 0.5f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerViewDistRatioPrediction, 1.f, VF_NULL, 
    "Debug");
  REGISTER_CVAR(e_VoxTerViewDistRatioMeshActivate, 0.5f, VF_NULL, 
    "Debug");
	REGISTER_CVAR(e_VoxelGenerate, 0, VF_NULL, 
		"Regenerate voxel world");
	REGISTER_CVAR(e_VoxelDebug, 0, VF_NULL, 
		"Draw voxel debug info");
	REGISTER_CVAR(e_VoxelAoRadius, 6, VF_NULL, 
		"Ambient occlusion test range in units");
	REGISTER_CVAR(e_VoxelAoScale, 8, VF_NULL, 
		"Ambient occlusion amount");
	REGISTER_CVAR(e_VoxelLodsNum, 2, VF_NULL, 
		"Generate LODs for voxels");

	REGISTER_CVAR(e_VoxelUpdatePhysics, 1, VF_NULL, 
		"Physicalize geometry");

	REGISTER_CVAR(e_ProcVegetation, 1, VF_NULL, 
		"Show procedurally distributed vegetation");
	REGISTER_CVAR(e_ProcVegetationMaxViewDistance, 128, VF_NULL, 
		"Maximum distance where procedural objects may be spawn");
  REGISTER_CVAR(e_ProcVegetationMaxSectorsInCache, 16, VF_NULL, 
    "Maximum number of 64x64 meter sectors cached in memory");
  REGISTER_CVAR(e_ProcVegetationMaxObjectsInChunk, 512, VF_NULL, 
    "Maximum number of instances per chunk");

	REGISTER_CVAR(e_Recursion, 1, VF_NULL, 
		"If 0 - will skip recursive render calls like render into texture");
  REGISTER_CVAR(e_RecursionOcclusionCulling,0, VF_NULL, 
		"If 0 - will disable occlusion tests for recursive render calls like render into texture");
  REGISTER_CVAR(e_RecursionViewDistRatio, .1f,VF_NULL, 
		"Set all view distances shorter by factor of X");

	REGISTER_CVAR(e_Clouds, 0, VF_NULL, 
		"Enable clouds rendering");

	REGISTER_CVAR(e_SkyUpdateRate, 0.12f, VF_NULL, 
		"Percentage of a full dynamic sky update calculated per frame (0..100].");	
	REGISTER_CVAR(e_SkyQuality, 1, VF_NULL, 
		"Quality of dynamic sky: 1 (very high), 2 (high).");
	REGISTER_CVAR(e_SkyType, 1, VF_NULL, 
		"Type of sky used: 0 (static), 1 (dynamic).");

	REGISTER_CVAR(e_DisplayMemoryUsageIcon, 1,VF_NULL,
		"Turns On/Off the memory usage icon rendering: 1 on, 0 off.");

	REGISTER_CVAR(e_LodRatio, 6.0f, VF_NULL, 
		"LOD distance ratio for objects");
  REGISTER_CVAR(e_LodCompMaxSize, 6.0f, VF_NULL, 
    "Affects LOD selection for big objects, small number will switch more objects into lower LOD");
	REGISTER_CVAR(e_ViewDistRatio, 60.0f, VF_NULL, 
		"View distance ratio for objects");
  REGISTER_CVAR(e_ViewDistCompMaxSize, 64.0f, VF_NULL, 
    "Affects max view distance for big objects, small number will render less objects");
  REGISTER_CVAR(e_ViewDistRatioPortals, 60.0f, VF_NULL, 
    "View distance ratio for portals");
	REGISTER_CVAR(e_ViewDistRatioDetail, 30.0f, VF_NULL, 
		"View distance ratio for detail objects");
	REGISTER_CVAR(e_ViewDistRatioVegetation, 30.0f, VF_NULL, 
		"View distance ratio for vegetation");
	REGISTER_CVAR(e_ViewDistCustomRatio, 100.0f, VF_CHEAT, 
		"View distance ratio for special marked objects (Players,AI,Vehicles)");
	REGISTER_CVAR(e_DebugLights, 0, VF_CHEAT, 
		"Use different colors for objects affected by different number of lights\n black:0, blue:1, green:2, red:3 or more, blinking yellow: more then the maximum enabled");
	REGISTER_CVAR(e_ViewDistMin, 0.0f, VF_CHEAT, 
		"Min distance on what far objects will be culled out");
	REGISTER_CVAR(e_LodMin, 0, VF_NULL, 
		"Min LOD for objects");
	REGISTER_CVAR(e_LodMax, 6, VF_CHEAT, 
		"Max LOD for objects");
	REGISTER_CVAR(e_LodMinTtris, 300, VF_CHEAT, 
		"LODs with less triangles will not be used");
  REGISTER_CVAR(e_TerrainAo, 1, VF_NULL, 
		"Activate deferred terrain ambient occlusion");

  REGISTER_CVAR(e_PhysMinCellSize, 2, VF_NULL, 
    "Min size of cell in physical entity grid");
	REGISTER_CVAR(e_PhysFoliage, 2, VF_NULL, 
		"Enables physicalized foliage\n"
		"1 - only for dynamic objects\n"
		"2 - for static and dynamic)");
	REGISTER_CVAR(e_FoliageStiffness, 3.2f, VF_NULL, 
		"Stiffness of the spongy obstruct geometry");
	REGISTER_CVAR(e_FoliageBranchesStiffness,	100.f, VF_NULL, 
		"Stiffness of branch ropes");
	REGISTER_CVAR(e_FoliageBranchesDamping, 10.f, VF_NULL, 
		"Damping of branch ropes");
	REGISTER_CVAR(e_FoliageBrokenBranchesDamping, 15.f, VF_NULL, 
		"Damping of branch ropes of broken vegetation");
	REGISTER_CVAR(e_FoliageBranchesTimeout, 4.f, VF_NULL, 
		"Maximum lifetime of branch ropes (if there are no collisions)");
	REGISTER_CVAR(e_FoliageWindActivationDist,	0,  VF_NULL, 
		"If the wind is sufficiently strong, visible foliage in this view dist will be forcefully activated");

  REGISTER_CVAR(e_DeformableObjects, 1, VF_NULL, 
		"Enable / Disable morph based deformable objects");

	REGISTER_CVAR(e_CullVegActivation, 200, VF_NULL, 
		"Vegetation activation distance limit; 0 disables visibility-based culling (= unconditional activation)");

	REGISTER_CVAR(e_PhysOceanCell, 0, VF_NULL, 
		"Cell size for ocean approximation in physics, 0 assumes flat plane");

	REGISTER_CVAR(e_JointStrengthScale, 1.0f, VF_NULL, 
		"Scales the strength of prebroken objects\' joints (for tweaking)");

	REGISTER_CVAR(e_VolObjShadowStrength,	0.4f, VF_NULL, 
		"Self shadow intensity of volume objects [0..1].");

	REGISTER_CVAR(e_ScreenShot, 0, VF_NULL, 
		"Make screenshot combined up of multiple rendered frames\n"
		"(negative values for multiple frames, positive for a a single frame)\n"
		" 1 highres\n"
		" 2 360 degree panorama\n"
		" 3 Map top-down view\n"
		"\n"
		"see:\n"
		"  e_ScreenShotWidth, e_ScreenShotHeight, e_ScreenShotQuality, e_ScreenShotMapCenterX,\n"
		"  e_ScreenShotMapCenterY, e_ScreenShotMapSize, e_ScreenShotMinSlices, e_ScreenShotDebug");
	
	REGISTER_CVAR(e_ScreenShotWidth, 2000, VF_NULL, 
		"used for all type highres screenshots made by e_ScreenShot to define the\n"
		"width of the destination image, 2000 default");
	REGISTER_CVAR(e_ScreenShotHeight, 1500, VF_NULL, 
		"used for all type highres screenshots made by e_ScreenShot to define the\n"
		"height of the destination image, 1500 default");
	REGISTER_CVAR(e_ScreenShotQuality, 30, VF_NULL, 
		"used for all type highres screenshots made by e_ScreenShot to define the quality\n"
		"0=fast, 10 .. 30 .. 100 = extra border in percent (soften seams), negative value to debug");
	REGISTER_CVAR(e_ScreenShotMinSlices, 4, VF_NULL, 
		"used for all type highres screenshots made by e_ScreenShot to define the amount\n"
		"of sub-screenshots for the width and height to generate the image,\n the min count\n"
		"will be automatically raised if not sufficient (per screenshot-based)");
	REGISTER_CVAR(e_ScreenShotMapCenterX, 0.0f, VF_NULL, 
		"param for the centerX position of the camera, see e_ScreenShotMap\n"
		"defines the x position of the top left corner of the screenshot-area on the terrain,\n"
		"0.0 - 1.0 (0.0 is default)");
	REGISTER_CVAR(e_ScreenShotMapCenterY, 0.0f, VF_NULL, 
		"param for the centerX position of the camera, see e_ScreenShotMap\n"
		"defines the y position of the top left corner of the screenshot-area on the terrain,\n"
		"0.0 - 1.0 (0.0 is default)");
	REGISTER_CVAR(e_ScreenShotMapSizeX, 1024.f, VF_NULL, 
		"param for the size in worldunits of area to make map screenshot, see e_ScreenShotMap\n"
		"defines the x position of the bottom right corner of the screenshot-area on the terrain,\n"
		"0.0 - 1.0 (1.0 is default)");
	REGISTER_CVAR(e_ScreenShotMapSizeY, 1024.f, VF_NULL, 
		"param for the size in worldunits of area to make map screenshot, see e_ScreenShotMap\n"
		"defines the x position of the bottom right corner of the screenshot-area on the terrain,\n"
		"0.0 - 1.0 (1.0 is default)");
	REGISTER_CVAR(e_ScreenShotMapCamHeight, 4000.f, VF_NULL, 
		"param for top-down-view screenshot creation, defining the camera height for screenshots,\n"
		"see e_ScreenShotMap defines the y position of the bottom right corner of the\n"
		"screenshot-area on the terrain,\n"
		"0.0 - 1.0 (1.0 is default)");
	REGISTER_CVAR(e_ScreenShotDebug, 0, VF_NULL, 
		"0 off\n1 show stitching borders\n2 show overlapping areas");

	REGISTER_CVAR(e_Ropes, 1, VF_CHEAT, 
		"Turn Rendering of Ropes on/off");

	int nDefaultValidate = 0;

#ifndef _RELEASE
	// Validate meshes in all but release builds.
	nDefaultValidate = 1;
#endif

  REGISTER_CVAR(e_StatObjValidate, nDefaultValidate, VF_NULL, 
     "Enable CGF mesh validation during loading");
  REGISTER_CVAR(e_StatObjPreload, 0, VF_NULL, 
    "Load level CGF's in efficient way");
  REGISTER_CVAR(e_StatObjMerge, 1, VF_NULL, 
		"Enable CGF sub-objects meshes merging");
  REGISTER_CVAR(e_StatObjMergeMaxTrisPerDrawCall, 500, VF_NULL, 
    "Skip merging of meshes already having acceptable number of triangles per draw call");

	REGISTER_CVAR(e_DefaultMaterial, 0, VF_CHEAT,
		"use gray illumination as default");

	REGISTER_CVAR(e_ObjQuality, 0, VF_NULL, 
		"Current object detail quality");
	REGISTER_CVAR(e_ParticlesQuality, 0, VF_NULL, 
		"Current particles detail quality");
  REGISTER_CVAR(e_ParticlesLights, 2, VF_NULL, 
		"Allows to have light source attached to every particle\n"
    "0 = Off\n"
    "1 = Deferred lights\n");

  REGISTER_CVAR(e_ParticlesLightsViewDistRatio, 256.0f, VF_NULL, 
    "Set particles lights view distance ratio");

	e_ScreenShotFileFormat = REGISTER_STRING("e_ScreenShotFileFormat", "tga",  VF_NULL,
		CVARHELP("Set output image file format for hires screen shots. Can be JPG or TGA"));

	REGISTER_CVAR(e_TimeOfDay,0.0f,VF_CHEAT,
		"Current Time of Day")->SetOnChangeCallback(OnTimeOfDayVarChange);
	REGISTER_CVAR(e_TimeOfDaySpeed,0.0f,VF_CHEAT,
		"Time of Day change speed")->SetOnChangeCallback(OnTimeOfDaySpeedVarChange);
  REGISTER_CVAR(e_TimeOfDayDebug, 0, VF_NULL, 
    "Display time of day current values on screen");
  

  REGISTER_CVAR(e_CameraRotationSpeed, 0, VF_CHEAT, 
    "Rotate camera around Z axis for debugging");
  REGISTER_CVAR(e_CameraFreeze, 0, VF_CHEAT, 
		"Freeze 3dengine camera (good to debug object culling and LOD).\n"
		"The view frustum is drawn in write frame.\n"
		" 0 = off\n"
		" 1 = activated");

  REGISTER_CVAR(e_SplitScreenTest, 0, VF_CHEAT, 
    "Split screen into 2x2 views. Currently doesn't work with deferred shading, post-processing, SSAO and terrain AO");

	REGISTER_CVAR(e_GI, 0, VF_NULL, 
    "Enable/disable global illumination. Default: 1 - enabled" );
	REGISTER_CVAR(e_GISecondaryOcclusion, 0, VF_NULL, 
		"Enable/disable secondary occlusion for global illumination. Default: 0 - disabled" );
	REGISTER_CVAR(e_GIGlossyReflections, 0, VF_NULL, 
		"Enable/disable reflective mode for global illumination. Default: 0 - disabled" );
	REGISTER_CVAR(e_GINumCascades, 1, VF_NULL, 
		"Sets number of cascades for global illumination. Default: 1" );
	REGISTER_CVAR(e_GICascadesRatio, 2.f, VF_NULL, 
		"Sets slope of cascades for global illumination. Default: 2.f" );
	REGISTER_CVAR(e_GIRSMSize, CREIrradianceVolume::espMaxInjectRSMSize, VF_NULL, 
    "Set the default reflective shadow map size.\n"
		"Default: 256 pixels for PC/128 for consoles, minimum: 64, maximum: 512" );
	REGISTER_CVAR(e_GIAmount, 1, VF_NULL, 
    "Multiplier for brightness of the global illumination. Default: 25.0 times brighter (temporary)" );
	REGISTER_CVAR(e_GIMaxDistance, 50, VF_NULL, 
    "Maximum distance of global illumination in meters.\n"
		"The less the distance the better the quality. Default: 50. Max: 150" );
#if !defined(XENON) && !defined(PS3)
	REGISTER_CVAR(e_GIIterations, 10, VF_NULL, 
		"Maximum number of propagation iterations global illumination\n"
		"The less number of propagation iterations the shorter the bleeding. Default: 6. Max: 32" );




#endif
	REGISTER_CVAR(e_GIOffset, .2f, VF_NULL, 
    "Offset of GI in front of camera in percents[0;1]. Default: 0.4 Min: 0 Max: 1" );

	REGISTER_CVAR(e_GIPropagationAmp, 3.3f, VF_NULL, 
		"Light amplification during each propagation iteration. Default: 3.3 Min: 1 Max: 5" );

	REGISTER_CVAR(e_GIBlendRatio, 0.25f, VF_NULL, 
		"Ratio of overlapped region between nested cascades. 0.25 means 25% overlapping. Default: 0.25 Min: .1 Max: 2" );

	REGISTER_CVAR(e_GICache, 7, VF_NULL, 
		"Sparse temporal caching for RSM rendering. Measured in framed per generation. Default: 7 Min: 0 (disabled)" );

  REGISTER_CVAR(e_StatsFlushPeriod, 0.0f, VF_NULL,
    "Time period for flush statistics");

  REGISTER_CVAR(e_WorldSegmentationTest,0, VF_CHEAT, 
    "Debug only: simulates multi-segment behavior in the editor");

	REGISTER_CVAR(e_PrepareDeformableObjectsAtLoadTime,0, VF_CHEAT, 
		"Enable to Prepare deformable objects at load time instead on demand, prevents peaks but increases memory usage");
}
