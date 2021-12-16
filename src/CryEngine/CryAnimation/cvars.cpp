//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:CVars.cpp
//  Implementation of CVars class
//
//	History:
//	September 23, 2004: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//need global var as the singleton approach is too expensive
Console g_Consoleself;
namespace 
{
	int g_ConsoleInstanceCount;
}

Console::Console()
{
	if(g_ConsoleInstanceCount++)
		abort();
}

void Console::Init()
{
//	ca_CharEditModel="Objects/characters/human/story/faraa/faraa.cdf";
	ca_CharEditModel="objects/characters/human/us/nanosuit_v2/nanosuit_v2_CharEdit.cdf";
//	ca_CharEditModel="objects/characters/neytiri/neytiri.cdf";

	ca_CharEditModel = "";
	// default SDK character model
	//ca_CharEditModel = "objects/characters/neutral_male/sdk_character_male.cdf";

	REGISTER_STRING( "ca_CharEditModel", ca_CharEditModel,	VF_CHEAT, CVARHELP("") );

	REGISTER_CVAR(ca_DrawPMGInfo,0,	VF_CHEAT,	"if set to 1, the PMG debug info is drawn"	);
	REGISTER_CVAR(ca_DrawBBox,0,	VF_CHEAT,	"if set to 1, the own bounding box of the character is drawn"	);
	REGISTER_CVAR(ca_DrawSkeleton, 0,	VF_CHEAT,	"if set to 1, the skeleton is drawn" );
	REGISTER_CVAR(ca_DrawDecalsBBoxes, 0,	VF_CHEAT,	"if set to 1, the decals bboxes are drawn"	);
	REGISTER_CVAR(ca_DrawPositionPre,	0,	VF_CHEAT,"draws the world position of the character (before update)");
	REGISTER_CVAR(ca_DrawPositionPost,	0,	VF_CHEAT, "draws the world position of the character (after update)");
	REGISTER_CVAR(ca_DrawEmptyAttachments, 0,	VF_CHEAT, "draws a wireframe cube if there is no object linked to an attachment");
	REGISTER_CVAR(ca_DrawWireframe, 0,	VF_CHEAT, "draws a wireframe on top of the rendered character");
	REGISTER_CVAR(ca_DrawAimPoses, 0,	VF_CHEAT, "draws the wireframe of the aim poses");
	REGISTER_CVAR(ca_DrawLookIK, 0, VF_CHEAT, "draws a visualization of look ik");
	REGISTER_CVAR(ca_DrawTangents, 0,	VF_CHEAT, "draws the tangents of the rendered character");
	REGISTER_CVAR(ca_DrawBinormals, 0,	VF_CHEAT, "draws the binormals of the rendered character");
	REGISTER_CVAR(ca_DrawNormals, 0,	VF_CHEAT,"draws the normals of the rendered character");
	REGISTER_CVAR(ca_DrawAttachments, 1,	VF_CHEAT, "if this is 0, will not draw the attachments objects");
	REGISTER_CVAR(ca_DrawFaceAttachments, 1,	0, "if this is 0, will not draw the skin attachments objects");

	REGISTER_CVAR(ca_DrawAttachmentOBB, 0,	VF_CHEAT, "if this is 0, will not draw the attachments objects"	);
	REGISTER_CVAR(ca_DrawBaseMesh, 1,	VF_CHEAT, "if this is 0, will not draw the characters");
	REGISTER_CVAR(ca_DrawMotionBlurTest, 0,	VF_CHEAT, "if this is 1, we draw the motion-blur test mesh"	);
	REGISTER_CVAR(ca_DrawLocator, 0,	0,	"if this is 1, we will draw the body and move-direction" );
	REGISTER_CVAR(ca_DrawIdle2MoveDir, 0,	0,	"if this is 1, we will draw the initial Idle2Move dir" );
	REGISTER_CVAR(ca_DrawFootPlants, 0,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, it will print some debug boxes at the feet of the character"	);
	REGISTER_CVAR(ca_DrawCGA, 1,	VF_CHEAT, "if this is 0, will not draw the CGA characters");
	REGISTER_CVAR(ca_DrawCC, 1,	VF_CHEAT, "if this is 0, will not draw the CC characters");
	REGISTER_CVAR(ca_DrawCGAAsSkin, 0,	0, "if this is 1, will draw the CGA characters using skin (dp calls decreased)");


	REGISTER_CVAR(ca_DebugText, 0,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, it will print some debug text on the screen" );
	REGISTER_CVAR(ca_DebugCommandBuffer, 0,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, it will print the amount of commands for the blend-buffer" );
	REGISTER_CVAR(ca_DebugSubstateTransitions, 0,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, it will be possible to test blending between sub-states of an AAC" );
	REGISTER_CVAR(ca_PrintDesiredSpeed, 0,	VF_CHEAT|VF_DUMPTODISK, "if this is 1, it will print the desired speed of the human characters"	);
	REGISTER_CVAR(ca_DebugFootPlants, 0,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, it will print some debug text on the screen"	);
	REGISTER_CVAR(ca_FootAnchoring, 0,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, it will print some debug boxes at the feet of the character"	);
	REGISTER_CVAR(ca_GroundAlignment, 1,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, the legs of human characters will align with the terrain"	);
	REGISTER_CVAR(ca_DebugAnimationStreaming, 0,	VF_CHEAT|VF_DUMPTODISK,	"if this is 1, then it shows what animations are streamed in" );


	REGISTER_CVAR(ca_SphericalSkinning,	1, VF_CHEAT|VF_DUMPTODISK,	"If this is 1, then use spherical-blend-skinning with up to 4 bones per vertex"	);

	REGISTER_CVAR(ca_SmoothStrafe,	1, VF_CHEAT,"If this is 1, then we smooth the strafe vector");
	REGISTER_CVAR(ca_LoadUncompressedChunks,	0, VF_CHEAT, "If this 1, then uncompressed chunks prefer compressed while loading");

	REGISTER_CVAR(ca_NoDeform, 0,	VF_CHEAT,	"the skinning is not performed during rendering if this is 1");
	REGISTER_CVAR(ca_UseMorph, 1,	VF_CHEAT,	"the morph skinning step is skipped (it's part of overall skinning during rendering)");
	REGISTER_CVAR(ca_NoAnim, 0,	VF_CHEAT,	"the animation isn't updated (the characters remain in the same pose)");
	REGISTER_CVAR(ca_UsePhysics, 1,	VF_CHEAT, "the physics is not applied (effectively, no IK)");
	REGISTER_CVAR(ca_UseLookIK, 1,	VF_CHEAT,	"If this is set to 1, then we are adding a look-at animation to the skeleton"	);


	REGISTER_CVAR(ca_UseAimIK,        1,	VF_CHEAT, "If this is set to 1, then we are adding a look-at animation to the skeleton");
	REGISTER_CVAR(ca_UseAimIKRefPose, 0, VF_CHEAT, 	"If this is set to 1, adjust aim pose by the reference pose if present" );
	REGISTER_CVAR(ca_AimIKFadeout,    1,	VF_NULL,	"if set to 0, the Aim-IK will not fade out when aiming in extreme directions"	);
	REGISTER_CVAR(ca_DrawAimIKVEGrid, 0,	VF_CHEAT,	"if set to 1, we will the the grid with the virtual examples"	);

	REGISTER_CVAR(ca_UseFacialAnimation, 1,	VF_CHEAT,	
		"If this is set to 1, we can play facial animations");
	REGISTER_CVAR(ca_FacialAnimationRadius, 30.0f, VF_CHEAT, 
		"Maximum distance at which facial animations are updated - handles zooming correctly");

	REGISTER_CVAR(ca_RandomScaling, 0,	VF_CHEAT, 
		"If this is set to 1, then we apply ransom scaling to characters");

	REGISTER_CVAR(ca_LockFeetWithIK, 1,	VF_CHEAT, 
		"If this is set to 1, then we lock the feet to prevent sliding when additive animations are used" 
		);

	REGISTER_CVAR(ca_UseAllJoints, 1,	VF_CHEAT, 
		"if set to 1, then have no Animation-LOD (debugging feature for animation LOD)");

	REGISTER_CVAR(ca_ForceUpdateSkeletons, 0, VF_CHEAT, "Always update all skeletons, even if not visible.");

	REGISTER_CVAR(ca_UseDecals, 1,	0,	"if set to 0, effectively disables creation of decals on characters\n2 - alternative method of calculating/building the decals" );
	REGISTER_CVAR(ca_DecalSizeMultiplier, 1.0f,	VF_CHEAT,"The multiplier for the decal sizes"	);

	REGISTER_CVAR(ca_DebugModelCache, 0,	VF_CHEAT,	"shows what models are currently loaded and how much memory they take");
	REGISTER_CVAR(ca_DebugAnimUpdates, 0,	VF_CHEAT,	"shows the amount of skeleton-updates" );
	REGISTER_CVAR(ca_DebugAnimUsage, 0,	VF_CHEAT,	"shows what animation assets are used in the level" );
	REGISTER_CVAR(ca_DumpUsedAnims, 0,	VF_CHEAT,	"writes animation asset statistics to the disk" );

	REGISTER_CVAR(ca_AnimWarningLevel, 3,	VF_CHEAT|VF_DUMPTODISK, 
		"if you set this to 0, there won't be any\nfrequest warnings from the animation system");
	REGISTER_CVAR(ca_KeepModels, 0,	VF_CHEAT, 
		"If set to 1, will prevent models from unloading from memory\nupon destruction of the last referencing character");



	// if this is not empty string, the animations of characters with the given model will be logged
	REGISTER_CVAR(ca_MemoryUsageLog, 0,	VF_CHEAT, "enables a memory usage log" );

	REGISTER_CVAR(ca_DebugSkeletonEffects, 0,	VF_CHEAT, "If true, dump log messages when skeleton effects are handled." );

	REGISTER_CVAR(ca_DeathBlendTime, 0.3f,	VF_CHEAT, "Specifies the blending time between low-detail dead body skeleton and current skeleton" );

	REGISTER_CVAR(ca_lipsync_phoneme_offset, 20,	VF_CHEAT, "Offset phoneme start time by this value in milliseconds" );

	REGISTER_CVAR(ca_lipsync_phoneme_crossfade, 70,	VF_CHEAT, "Cross fade time between phonemes in milliseconds" );

	REGISTER_CVAR(ca_lipsync_vertex_drag, 1.2f,	VF_CHEAT, "Vertex drag coefficient when blending morph targets" );

	//REGISTER_CVAR(ca_lipsync_phoneme_strong, 0.75f, 0, "Strong phoneme intensity border" );
	//REGISTER_CVAR(ca_lipsync_phoneme_weak, 0.25f, 0, "Weak phoneme intensity border" );

	REGISTER_CVAR(ca_eyes_procedural, 1, 0, "Enables/Disables procedural eyes animation" );
	//REGISTER_CVAR(ca_face_procedural_strength, 1.0f, 0, "Procedural face animation strength" );
	//REGISTER_CVAR(ca_face_procedural_timing, 1.0f, 0, "Procedural face animation strength" );

	REGISTER_CVAR(ca_lipsync_phoneme_strength, 1.0f, 0, "LipSync phoneme strength" );
	REGISTER_CVAR(ca_lipsync_debug, 0,	VF_CHEAT,	"Enables facial animation debug draw" );
	REGISTER_CVAR(ca_DebugFacial, 0,	VF_CHEAT,	"Debug facial playback info"	);
	REGISTER_CVAR(ca_DebugFacialEyes, 0, VF_CHEAT, "Debug facial eyes info" );


	int startVal = 0;
	// Ivo, please not comment next time. This is important. If you need to 
#if defined(XENON) || defined(PS3)
	startVal = 1;
#endif
	REGISTER_CVAR(ca_LoadPriority, startVal, VF_CHEAT, "Change file loading priority (0 - first file, 1- first dba)");
	REGISTER_CVAR(ca_SkipLoadThinFat, startVal, VF_CHEAT, "Skip loading fat\thin stuff if enabled");

	REGISTER_CVAR(ca_ForceNullAnimation, 0,	VF_CHEAT,	"replaces the mocap-data by default position" );

	REGISTER_CVAR(ca_travelSpeedScaleMin, 0.5f,	VF_CHEAT,	"Minimum motion travel speed scale (default 0.5)." );
	REGISTER_CVAR(ca_travelSpeedScaleMax, 2.0f,	VF_CHEAT,	"Maximum motion travel speed scale (default 2.0)." );

	REGISTER_CVAR(ca_GameControlledStrafing, 1, VF_CHEAT,	"Use game controlled strafing/curving flag, instead of low level calculated curving weight." );

	REGISTER_CVAR(ca_DebugADIKTargets, 0,	VF_CHEAT,	"If 1, then it will show if there are animation-driven IK-Targets for this model." );
	REGISTER_CVAR(ca_DebugCaps, 0,	0,	"Display current blended motion capabilities." );
	REGISTER_CVAR(ca_SaveAABB, 0,	0,	"if the AABB is invalid, replace it by the default AABB" );
	REGISTER_CVAR(ca_AttachmentCullingRation, 200.0f,	0,	"ration between size of attachment and distance to camera" );
	REGISTER_CVAR(ca_FPWeaponInCamSpace, 1,	0,	"if this is 1, then we attach the wepon to the camera" );

	REGISTER_CVAR(ca_DatabaseUnloading, 1,	VF_NULL,	"if 1, then unload DBA if not used");
	REGISTER_CVAR(ca_UnloadAimPoses,   1,	VF_DUMPTODISK,	"remove aim-poses from memory" );
	REGISTER_CVAR(ca_UnloadAnimations, 1,	VF_DUMPTODISK,	"unloading streamed animation as soon as they are not used" );


	REGISTER_CVAR(ca_UseAnimationsCache, 0,	VF_DUMPTODISK,	"use dynamically unpacked animations cache" );

	g_pIConsole->Register( "ca_testInt", &ca_testInt, 0, 0, "integer number for testing purposes" );
	g_pIConsole->Register( "ca_testFloat", &ca_testFloat, 0, 0, "floating-point number for testing purposes" );

	static ICVar *p_e_lod_ratio = g_pISystem->GetIConsole()->GetCVar("ca_lod_ratio");
	float eLod = 6.0f;
	if (p_e_lod_ratio)
		eLod = p_e_lod_ratio->GetFVal();

	REGISTER_CVAR(ca_lod_ratio, eLod,	VF_DUMPTODISK,	"Character LOD ratio" );

	g_pIConsole->Register( "ca_fullAnimStatistics", &ca_fullAnimStatistics, 0, 0, 
		"If 1, animation statistics shows all animations LMG is consist of, but does not show LMG names themselves.\n" \
		"If 0, shows only top-level LMG names" );


	m_pMotionBlurShutterSpeed = g_pIConsole->GetCVar("r_MotionBlurShutterSpeed");  
	m_pMotionBlurFrameTimeScale = g_pIConsole->GetCVar("r_MotionBlurFrameTimeScale");  
	m_pMotionBlur = g_pIConsole->GetCVar("r_MotionBlur");  


	//------------------------------------------------------------


	REGISTER_CVAR(ca_EnableAssetStrafing, 1,	0,	"asset strafing is disabled by default" );
	REGISTER_CVAR(ca_EnableAssetTurning, 1,	0,	"asset tuning is disabled by default" );
	REGISTER_CVAR(ca_AMC, 1,	0,	"enable some AMC functionality" );
	//	REGISTER_CVAR(ca_AMC_TurnSpeed, 1.1f,	0,	"turn faster-slowe" );
	REGISTER_CVAR(ca_AMC_TurnLeaning, 0.35f,	0,	"lean stronger in curves" );

	REGISTER_CVAR(ca_cloth_vars_reset, 2,	0,	"1 - load the values from the next char, 1 - apply normally, 2+ - ignore" );
	REGISTER_CVAR(ca_cloth_max_timestep, 0.0f, 0,	"" );
	REGISTER_CVAR(ca_cloth_max_safe_step, 0.0f, 0,	"if a segment stretches more than this (in *relative* units), its length is reinforced" );
	REGISTER_CVAR(ca_cloth_stiffness, 0.0f, 0,	"stiffness for stretching" );
	REGISTER_CVAR(ca_cloth_thickness, 0.0f, 0,	"thickness for collision checks" );
	REGISTER_CVAR(ca_cloth_friction, 0.0f, 0,	"" );
	REGISTER_CVAR(ca_cloth_stiffness_norm, 0.0f, 0,	"stiffness for shape preservation along normals (\"convexity preservation\")" );
	REGISTER_CVAR(ca_cloth_stiffness_tang, 0.0f, 0,	"stiffness for shape preservation against tilting" );
	REGISTER_CVAR(ca_cloth_damping, 0.0f, 0,	"" );
	REGISTER_CVAR(ca_cloth_air_resistance, 0.0f, 0,	"\"advanced\" (more correct) version of damping" );

	// Command Buffer
	REGISTER_CVAR(ca_UseJointMasking, 1,	VF_DUMPTODISK,	"Use Joint Masking to speed up motion decoding.");

	// Multi-Threading
	REGISTER_CVAR(ca_disable_thread, 1, VF_DUMPTODISK, "TEMP Disable Animation Thread.");
	REGISTER_CVAR(ca_thread, 1, VF_DUMPTODISK, "If >0 enables Animation Multi-Threading.");
	REGISTER_CVAR(ca_thread0Affinity, 5, VF_DUMPTODISK, "Affinity of first Animation Thread.");
	REGISTER_CVAR(ca_thread1Affinity, 3, VF_DUMPTODISK, "Affinity of second Animation Thread.");

	REGISTER_CVAR(ca_testDoubleBuffering, 0, VF_DUMPTODISK, "Double-Buffering test.");
	
	REGISTER_CVAR(ca_AlienLegIK, 1,	VF_DUMPTODISK,	"Option for alien-leg IK, 1 for default and 2 for the other" );

}



