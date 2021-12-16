// "Uber" file for forward kinematics job (speeds up scan time)
#if defined(__SPU__)

// macro to activate Cryname.c_str workaournd(so that it not interferes with other jobs)
#define PROVIDE_GLOBAL_EMPTYSTYRING_FOR_SPU

#include <platform.h>

SPU_LOCAL unsigned int gControllerLookupTableSize;
SPU_LOCAL SPU_DOMAIN_LOCAL unsigned int	gControllerLookup[512] _ALIGN(128);

// store number of joints on spu storage to spare mainloopups everytime the numjoints is computed
SPU_LOCAL uint32 g_jointCount;

// Global vars needed for fast controller lookup on spu
SPU_LOCAL uint32 gCurrentLookupTableAddr;

class CAnimationManager;
// change g_AnimationManager macro to spare main memory lookup on spu
SPU_LOCAL SPU_DOMAIN_MAIN CAnimationManager*	g_pAnimationManager;

#include "stdafx.h"  // For SPU defines
__CRYCG_PRESERVE__ struct ShadowMapFrustum; 

// ========================================= //
// == common stuff for all animation jobs == //
// ========================================= //

// use char array to prevent code generate for data section(cause of global constructors)
SPU_LOCAL char gSkeletonAbsolutPoseStorage[MAX_JOINT_AMOUNT * sizeof(QuatT)] _ALIGN(128);
SPU_LOCAL char gSkeletonRelativePoseStorage[MAX_JOINT_AMOUNT * sizeof(QuatT)] _ALIGN(128);
SPU_LOCAL char gSkeletonControllerInfoStorage[MAX_JOINT_AMOUNT * sizeof(Status4)] _ALIGN(128);

// global vars for SPU Jobs (used to communicate arrays over function boundaries)
SPU_LOCAL SPU_DOMAIN_LOCAL QuatT 					*gSkeletonAbsolutePose;
SPU_LOCAL SPU_DOMAIN_LOCAL QuatT 					*gSkeletonRelativePose;
SPU_LOCAL SPU_DOMAIN_LOCAL Status4 				*gSkeletonControllerInfo;

SPU_LOCAL SPU_DOMAIN_LOCAL Skeleton::CLocator* gBlendedRoot;

// ===============================================//
// == global vars for ProcessForwardKinematics == //
// ============================================== //
// used dma ids (also 3,4,5,6 are used but the macros for these must be visible on ppu so they are defined in XXX.cpp)
#define DMA_ABSOLUTEPOSE_ID 0
#define DMA_RELATIVEPOSE_ID 1
#define DMA_CONTROLLERINFO_ID 2



// ============================================ //
// == global vars for ProcessAnimationUpdate == //
// ============================================ //
//used dma ids
#define ASYNC_ANIM_DMA_1	0
#define ASYNC_ANIM_DMA_2	1
#define ASYNC_ANIM_DMA_3	2
#define ANIM_BACK_TRANSFER	3

// arrays to cache the AnimationFifos during SPU executation
// since layer0 is references from later layers it gets a special treatment.
SPU_LOCAL SPU_DOMAIN_LOCAL CAnimation *gAninmationFifo;
SPU_LOCAL SPU_DOMAIN_LOCAL CAnimation *gAninmationFifoLayer0;

// utility structure for more cachefriendly loading of some structures
// instead of accessing these fields in main memory per reference, all relevant fields
// are transfered to the stack and read from there
struct GlobalAnimationHeaderType
{
	GlobalAnimationHeaderType( const GlobalAnimationHeaderCAF &rGAH ) :
		m_vVelocity(rGAH.m_vVelocity),
		m_fTotalDuration(rGAH.m_fTotalDuration),
		m_fSpeed(rGAH.m_fSpeed),
		m_fTurnSpeed(rGAH.m_fTurnSpeed)
	{}

	Vec3 m_vVelocity;
	f32 m_fTotalDuration;
	f32 m_fSpeed;
	f32 m_fTurnSpeed;
} _ALIGN(32); 

ILINE uint32 cry_rand32()
{
	return gSpuRandomNumberGen.Generate();
}
ILINE unsigned int cry_rand()
{	
	return gSpuRandomNumberGen.Generate() & RAND_MAX; 
}
ILINE float cry_frand()
{	
	return gSpuRandomNumberGen.GenerateFloat();
}

// ============================================//
// == includes for ProcessForwardKinematics == //
// =========================================== //
#include "SkeletonAnim.cpp"						// Entry Functions
#include "SkeletonPose.cpp"	
#include "SkeletonPose_IK2B.cpp"

#include "Skeleton.cpp"

#include "Command_Commands.cpp"
#include "Command_Buffer.cpp"
#include "SkeletonAnim_Commands.cpp"

#include "PoseModifier/PoseModifierHelper.cpp"
#include "PoseModifier/FeetLock.cpp"
#include "PoseModifier/LookAt.cpp"
#include "PoseModifier/PoseBlenderAim.cpp"
#include "PoseModifier/PoseBlenderAim2.cpp"

#include "ControllerPQ.cpp" 
#include "ControllerPQLog.cpp"

#include "AttachmentManager.cpp"

// check for scan optimization

#include UNIQUE_VIRTUAL_WRAPPER(ICharacterModel)
#include UNIQUE_VIRTUAL_WRAPPER(ISkeletonPose)
#include UNIQUE_VIRTUAL_WRAPPER(ICharacterModelSkeleton)
#include "SkinInstance.cpp"	// virtual function...
#include "FacialAnimation/FacialInstance.cpp" // ApplyProceduralFaceBehaviour
#include "../CrySystem/Timer.cpp" // for one small virtual timer function 

// ==========================================//
// == includes for ProcessAnimationUpdate == //
// ========================================= //
		//#include "SkeletonAnim.cpp"					// Entry Functions
#include "SkeletonAnim_BlendMan.cpp"			// Layer Blend functions
	#include "ModelAnimationSet.cpp"				// getModelAnimationName
#include "LMG_ComputeWeight.cpp"
//#include "LMG_ExtractParameters.cpp"
#include "LMG_GetCapabilities.cpp"
#include "SkeletonAnim_Params.cpp"				// set/update MotionParam....




#endif // __SPU__
