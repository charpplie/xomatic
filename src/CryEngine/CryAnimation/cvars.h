//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:CVars.h
//  Implementation of CVars class
//
//	History:
//	September 23, 2004: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
# pragma once

#ifndef _CRY_ANIMATION_CVARS_H_
#define _CRY_ANIMATION_CVARS_H_


//////////////////////////////////////////////////////////////////////////
// console variable interfaces.
// In an instance of this class (singleton for now), there are all the interfaces
// of all the variables needed for the Animation subsystem.
//////////////////////////////////////////////////////////////////////////
struct Console;
extern Console g_Consoleself;

struct Console
{
	Console();
	//The base class of the character-manager is a SINGLETON object
	//Its impossible to remove this object or to create another instance of the console
	static ILINE Console& GetInst()
	{		
		return g_Consoleself;
	}

	enum {nDefaultAnimWarningLevel = 2};

	void Init();
	const char* ca_CharEditModel;

	int ca_AlienLegIK;
	int ca_AimIKFadeout;
	int ca_DrawAimIKVEGrid;

	int ca_DrawPMGInfo;
	int ca_DrawAimPoses;
	int ca_DrawBBox;
	int ca_DrawSkeleton;
	int ca_DrawDecalsBBoxes;
	int ca_DrawPositionPre;
	int ca_DrawPositionPost;
	int ca_DrawEmptyAttachments;
	int ca_DrawLookIK;
	int ca_PrintDesiredSpeed;

	int ca_LoadPriority;
	int ca_SkipLoadThinFat;

	int ca_DrawWireframe;

	int ca_DrawTangents;
	int ca_DrawBinormals;
	int ca_DrawNormals;
	int ca_DrawFootPlants;
	int ca_FootAnchoring;
	int ca_GroundAlignment;

	int ca_DrawAttachments;
	int ca_DrawFaceAttachments;
	int ca_DrawAttachmentOBB;
	int ca_DrawBaseMesh;
	int ca_DrawMotionBlurTest;
	int ca_DrawLocator;
	int ca_DrawIdle2MoveDir;
	int ca_DrawCGA;
	int ca_DrawCGAAsSkin;
	int ca_DrawCC;

	int ca_DebugText;
	int ca_DebugCommandBuffer;
	int ca_DebugSubstateTransitions;
	int ca_DebugFacial;
	int ca_DebugFacialEyes;
	int ca_DebugFootPlants;
	int ca_DebugAnimationStreaming;

	int ca_SmoothStrafe;
	int ca_SphericalSkinning;
	int ca_NoDeform;
	int ca_UseMorph;
	int ca_NoAnim;
	int ca_UsePhysics;
	int ca_UseLookIK;
	int ca_UseAimIK;
	int	ca_UseAimIKRefPose;
	int ca_UseFacialAnimation;
	f32 ca_FacialAnimationRadius;
	int ca_RandomScaling;
	int ca_UseAllJoints;
	int ca_ForceUpdateSkeletons;

	int ca_UseDecals;
	f32 ca_DecalSizeMultiplier;
	f32 ca_LockFeetWithIK;

	int ca_AnimWarningLevel;
	int ca_KeepModels;

	int ca_DebugModelCache;
	int ca_DatabaseUnloading;
	int ca_DebugAnimUpdates;
	int ca_DebugAnimUsage;

	int ca_DumpUsedAnims;

	int ca_LoadUncompressedChunks;


	f32 ca_DeathBlendTime;

	int ca_eyes_procedural;
	//float ca_face_procedural_strength;
	//float ca_face_procedural_timing;
	int ca_lipsync_phoneme_offset;
	int ca_lipsync_phoneme_crossfade;
	float ca_lipsync_vertex_drag;
	//float ca_lipsync_phoneme_strong;
	//float ca_lipsync_phoneme_weak;
	float ca_lipsync_phoneme_strength;
	int ca_lipsync_debug;
	int ca_ForceNullAnimation;

	float ca_travelSpeedScaleMin;
	float ca_travelSpeedScaleMax;

	int ca_GameControlledStrafing;

	int	ca_DebugADIKTargets;
	int ca_DebugCaps;
	int ca_UnloadAimPoses;
	int ca_UnloadAnimations;
	int ca_SaveAABB;
	int ca_FPWeaponInCamSpace;
	f32 ca_AttachmentCullingRation;

	int ca_UseAnimationsCache;

	int ca_MemoryUsageLog;

	int ca_DebugSkeletonEffects;


	ICVar *m_pMotionBlurShutterSpeed;
	ICVar *m_pMotionBlurFrameTimeScale;
	ICVar *m_pMotionBlur;

	float ca_lod_ratio;
	//	int ca_lod_min;



	int ca_EnableAssetTurning;
	int ca_EnableAssetStrafing;
	int ca_AMC;
	//	f32 ca_AMC_TurnSpeed;
	f32 ca_AMC_TurnLeaning;

	int ca_cloth_vars_reset;
	float ca_cloth_max_timestep;
	float ca_cloth_max_safe_step;
	float ca_cloth_stiffness;
	float ca_cloth_thickness;
	float ca_cloth_friction;
	float ca_cloth_stiffness_norm;
	float ca_cloth_stiffness_tang;
	float ca_cloth_damping;
	float ca_cloth_air_resistance;

	int ca_testInt;
	float ca_testFloat;
	int ca_fullAnimStatistics;

	// Command Buffer
	int ca_UseJointMasking;

	// Multi-Threading
	int ca_disable_thread;
	int ca_thread;
	int ca_thread0Affinity;
	int ca_thread1Affinity;

	int ca_testDoubleBuffering;

	//////////////////////////////////////////////////////////////////////////
private:

//	~Console() {};
//	Console(const Console&);
	void operator=(const Console&);

} _ALIGN(128);


#endif // _3DENGINE_CVARS_H_
