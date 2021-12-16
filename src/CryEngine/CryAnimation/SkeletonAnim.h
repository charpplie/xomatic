//////////////////////////////////////////////////////////////////////
//
// CryEngine Source code
//
// File:SkeletonAnim.h
// Implementation of Skeleton class
//
// History:
// August 6, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _SKELETON_ANIM_H
#define _SKELETON_ANIM_H



#include <SpuUtils.h>
#include "GlobalAnimationHeaderPMG.h"
#include "AnimationThreadTask.h"
class CSkeletonAnim;
class CSkeletonPose;
class CCharInstance;
struct ModelAnimationHeader;

namespace Command
{
	class CBuffer;
}

#define numVIRTUALLAYERS (0x10)
/*
struct Locator
{
	//Quat m_InitJoints[5];  //just for Aim-IK
	SFootPlant m_FootPlant;
	uint32 m_nFootBits;

	f32	  m_fCurrentSpeed;
	f32		m_fCurrentTurn;
	f32		m_fCurrentStreife;
	f32		m_fCurrentSlope;  //we need to redefine this

	Vec3	m_vCurrentVelocity;
	Vec3	m_vRelTranslation;
	f32		m_fRelRotation;

	Locator() {	Init();	}

	void Init()
	{

		m_nFootBits = 0;

		m_fCurrentSpeed		= 0.0f;
		m_fCurrentTurn		= 0.0f;
		m_fCurrentSlope		= 0.0f;
		m_fCurrentStreife = 0.0f;

		m_vCurrentVelocity = Vec3(ZERO);
		m_fRelRotation=0;
		m_vRelTranslation = Vec3(ZERO);

	//	uint32 numAddJoints = sizeof(m_InitJoints)/sizeof(Quat);
	//	for (uint32 i=0; i<numAddJoints; i++)
	//		m_InitJoints[i].SetIdentity();
	};

}_ALIGN(128);
*/
// queue used for threading to prevent resizing of m_arrFIFO vectors while computations are running on these
struct DeferredQueueUpdate
{
	DeferredQueueUpdate( bool bRemoveFirstAnim, int32 nLayer, const CAnimation &rAnim ) :
		m_bRemoveFirstAnim(bRemoveFirstAnim), m_nLayer(nLayer), m_Anim(rAnim) {}

	void GetMemoryUsage(ICrySizer *pSizer ) const{}

	CAnimation m_Anim;
	int32 m_nLayer;
	bool m_bRemoveFirstAnim;
};
 
class CSkeletonAnimTask :
	public CAnimationThreadTask
{
private:
	static IThreadTaskManager* GetThreadTaskManager() { return gEnv->pSystem->GetIThreadTaskManager(); }

public:
	CSkeletonAnimTask();
	~CSkeletonAnimTask();

public:
	void Initialize(CSkeletonAnim& skeletonAnim);

	void Begin(const QuatT& locationPhysics, const QuatTS& locationAnimation);
	void Wait();

	// CAnimationThreadTask
public:
	void Execute();

private:
	bool m_executeNeeded;
//	CAnimationThreadEvent m_executeEvent;
	CryEvent m_executeEvent;

	bool m_bExecuted;
	CryCriticalSection m_executed;

	CSkeletonAnim* m_pSkeletonAnim;
	QuatT m_locationPhysics;
	QuatTS m_locationAnimation;
};

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
class CSkeletonAnim : public ISkeletonAnim
{
public:
	CSkeletonAnim() {} 

	bool bExtrapolation;
	f32 controlParam[MAX_PMG_PARAM_DIM+1]; // Need one additional dimension for homogeneous coordinates
	string controlParamName[MAX_PMG_PARAM_DIM+1]; // The corresponding name for each control parameter
	bool IsPMG_Extrapolation() const {return bExtrapolation;}

	uint32 GetToltalPosKeys(const bool isCAF, const uint32 nGlobalAnimID) const
	{
		if(!isCAF)
			return 0;
		else
		{
			GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
			return rGlobalAnimHeader.GetTolalPosKeys();
		}
	}

	uint32 GetToltalRotKeys(const bool isCAF, const uint32 nGlobalAnimID) const
	{
		if(!isCAF)
			return 0;
		else
		{
			GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
			return rGlobalAnimHeader.GetTolalRotKeys();
		}
	}

	size_t GetAnimationSize(const bool isCAF, const bool isLMG, const bool isPMG, const uint32 nGlobalAnimID)
	{
		if(isLMG)
		{
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID];
			return rGlobalAnimHeader.SizeOfLMG();
		}
		else if(isPMG)
		{
			GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID];
			return rGlobalAnimHeader.SizeOfThis();
		}
		else 
		{
				GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
				return rGlobalAnimHeader.SizeOfCAF(true);
		}		
	}
	void SetControlParam(const f32 param, const char* name, const int32 i) 
	{ 
		assert(i <= MAX_PMG_PARAM_DIM); 
		controlParam[i] = param; 
		controlParamName[i] = string(name);
	}

	f32 GetControlParam(const char* name) 
	{
		for(uint32 i=0; i<MAX_PMG_PARAM_DIM; ++i)
		{
			if(stricmp(controlParamName[i], name) == 0)
				return controlParam[i];
		}

		return controlParam[0];
	}
	void ResetControlParam() 
	{
		//------------------------------------------------------------------------------
		// Reset the control parameters

		for(uint32 i=0; i<MAX_PMG_PARAM_DIM; ++i)
		{
			controlParam[i] = 0.0f;
			controlParamName[i] = "";
		}
		
		controlParam[MAX_PMG_PARAM_DIM] = 1.0f;
		
	}

	void TestDrawParams(const std::vector< std::vector<f32> >& visParams, ColorB* cb, const Vec3& controlP, const uint32 numAsserts, f32* weights);

	size_t SizeOfThis (ICrySizer * pSizer);
	void GetMemoryUsage(ICrySizer * pSizer) const;
	void Serialize(TSerialize ser);

	void SetCharEditMode( uint32 m );

	// no editor on ps3, so no need for code related to editor
#if defined(PS3)
	ILINE bool GetCharEditMode() { return false; };
#else
	bool GetCharEditMode() { return m_CharEditMode; };
#endif

	void GetCapabilities_PMG(const SParametric & pmg, LMGCapabilities & lmg_caps);
	void WeightComputation_PMG(SParametric & pmg, CAnimationSet * pAnimationSet);
	struct TWarp
	{
		f32 m_fDuration;
		f32 m_fSpeed;
		f32 m_fDistance;
		TWarp()
		{
			m_fDuration=0;
			m_fSpeed=0;
			m_fDistance=0;
		}
	};
	TWarp GetTimewarpedDuration_PMG(CAnimationSet* pAnimationSet, SParametric& pmg);
	void CheckBlendWeights_PMG(CAnimationSet* pAnimationSet, SParametric& pmg);


	void SetAnimationDrivenMotion(uint32 ts);
	void SetMirrorAnimation(uint32 ts);
	uint32 GetAnimationDrivenMotion() { return m_AnimationDrivenMotion; };

	void SetTrackViewExclusive(uint32 i)
	{
		if (m_TrackViewExclusive && !i)	m_bReinitializeAnimGraph = true;
		m_TrackViewExclusive= i > 0;
	};
	uint32 GetTrackViewStatus() {	return m_TrackViewExclusive;	};


	bool StartAnimation(const char* szAnimName0, const CryCharAnimationParams& Params);
	bool StartAnimationById(int32 id, const CryCharAnimationParams& Params);

	bool StopAnimationInLayer(int32 nLayer, f32 BlendOutTime);
	bool StopAnimationsAllLayers();

	uint32 AnimationToQueue( const ModelAnimationHeader* pAnim0,int a0, const ModelAnimationHeader* pAnimAim0,int aid0,const ModelAnimationHeader* pAnimAim1,int aid1, f32 btime, const CryCharAnimationParams& AnimParams);

	void ClearFIFOLayer( uint32 nLayer );
	void UnloadAnimationAssets(DynArray<CAnimation>& arrAFIFO, int num);
	int GetNumAnimsInFIFO(uint32 nLayer) { return m_arrLayer_AFIFO[nLayer].size(); };
	bool RemoveAnimFromFIFO(uint32 nLayer, uint32 num);
	CAnimation& GetAnimFromFIFO(uint32 nLayer, uint32 num )
	{
#if defined(__SPU__) // crycg workaround for multiple constructor calls
		CAnimation *ret = &g_DefaultAnim;
		uint32 numAnimsInFifo = m_arrLayer_AFIFO[nLayer].size();
		if (num<numAnimsInFifo)
			ret = &m_arrLayer_AFIFO[nLayer][num];

		return *ret;
#else
		uint32 numAnimsInFifo = m_arrLayer_AFIFO[nLayer].size();
		if (num>=numAnimsInFifo)
			return g_DefaultAnim;
		return m_arrLayer_AFIFO[nLayer][num];
#endif
	}
	CAnimation* FindAnimInFIFO( uint32 nUserToken, int nLayer=1, bool includeDeferredAnimation = false );
	void ManualSeekAnimationInFIFO(uint32 nLayer, uint32 num, float time, bool advance);

	// [*DavidR | 22/Mar/2010] BEGIN: 
	// TEMPORARY METHODS UNTIL m_arrDeferredQueueUpdates GETS REMOVED
	VIRTUAL int  GetNumAnimsInFIFOAndDeferredQueue(uint32 nLayer);
	VIRTUAL CAnimation& GetAnimFromFIFOAndDeferredQueue(uint32 nLayer, uint32 num);
	// [*DavidR | 22/Mar/2010] END 


	//! Set the current time of the given layer, in seconds
	void SetLayerTime (uint32 nLayer, float fTimeSeconds);
	//! Return the current time of the given layer, in seconds
	float GetLayerTime (uint32 nLayer) const;

	// sets the animation speed scale for layers
	void SetLayerUpdateMultiplier(int32 nLayer, f32 fSpeed);
	f32 GetLayerUpdateMultiplier(uint32 nLayer)
	{
		if (nLayer<numVIRTUALLAYERS)
			return m_arrLayerSpeedMultiplier[nLayer];
		return 1.0f;
	}

	void SetAdditiveWeight(int32 nLayer, f32 fWeight);
	void SetLayerBlendMultiplier(int32 nLayer, f32 fMult);

	ILINE uint8 GetActiveLayer(uint8 layer)
	{
		return (m_ActiveLayer >> layer) & 1;
	}

	ILINE void SetActiveLayer(uint8 layer, uint8 val)
	{
		uint32 mask2 = 1 << layer;
		uint32 mask = (val & 0x1) << layer;
		m_ActiveLayer = (m_ActiveLayer & ~mask2) | mask;
	}


	VIRTUAL void SetDesiredLocalLocation(const QuatT& desiredLocalLocation, float deltaTime, float frameTime, float turnSpeedMultiplier);
	VIRTUAL void SetDesiredMotionParam(EMotionParamID id, float value, float deltaTime, bool initOnly = false);
	VIRTUAL void SetDesiredMotionParam2(EMotionParamID id, float value, float deltaTime, bool initOnly = false);
	VIRTUAL float GetDesiredMotionParam(EMotionParamID id) const;
	VIRTUAL void SetBlendSpaceOverride(EMotionParamID id, float value, bool enable); // Enable/Disable direct override of blendspace weights, used from CharacterEditor.


	void SetDesiredMotionParamsFromDesiredLocalLocation(float frameTime);
	void SetClampedMotionParam(SParametric& lmg, EMotionParamID id, float newValue, float deltaTime, bool initOnly);

	void UpdateMotionParamDescs(SParametric& lmg, float transitionBlendWeight);
	void ConvertLMGCapsToMotionParamDesc(EMotionParamID id, MotionParamDesc& desc, const LMGCapabilities& caps);
  void UpdatePMGParameters(const GlobalAnimationHeaderPMG* pmgHeader, SParametric& pmg);
	void UpdateMotionParamBlendSpacesForActiveMotions(float deltaTime);
	void UpdateMotionBlendSpace(SParametric& lmg, float deltaTime, float transitionBlendWeight);
	void UpdateMotionParamBlendSpace(SParametric& lmg, EMotionParamID id); // Update blendspace for the given parameter
	void CalculateMotionParamBlendSpace(MotionParam& param, float undefined); // Helper function to UpdateMotionParamBlendSpace

	// Converts new blendspace into old blendspace (and updates/interpolates strafe vector)
	void UpdateOldMotionBlendSpace(SParametric& lmg, float frameTime, float transitionBlendWeight);

	// Converts new blendspace into old blendspace (and updates/interpolates strafe vector)
	void UpdateOldMotionBlendSpace(SParametric& lmg, float frameTime);

	void ProcessAnimations(  const QuatT& rPhysLocationCurr, const QuatTS& rAnimCharLocationCurr, uint32 OnRender );
	uint32 BlendManager (f32 deltatime, DynArray<CAnimation>& arrLayer, uint32 nLayer );
	uint32 BlendManagerDebug ( DynArray<CAnimation>& arrLayer, uint32 nLayer );
	void LayerBlendManager( f32 fDeltaTime, uint32 nLayer );
	void UpdateAndPushIntoExecBuffer( CAnimation *arrAFIFO, uint32 nLayer, uint32 NumAnimsInQueue, uint32 AnimNo );


	QuatT m_desiredLocalLocation;
	float m_desiredArrivalDeltaTime;
	float m_desiredTurnSpeedMultiplier; // Makes players turn/catchup faster, being more responsive to quick turns.
	f32 m_fDesiredTurnSpeedOffset;
	f32 m_fDesiredTurnSpeedScale;
	f32 m_fDesiredTurnSpeedBlend;
	// Enable/Disable direct override of blendspace weight values, primarily used from CharacterEditor.
	float m_desiredMotionParam[eMotionParamID_COUNT];
	bool m_CharEditBlendSpaceOverrideEnabled[eMotionParamID_COUNT];
	float m_CharEditBlendSpaceOverride[eMotionParamID_COUNT];

	DynArray<DeferredQueueUpdate> m_arrDeferredQueueUpdates; 
	DynArray<CAnimation> m_arrLayer_AFIFO[numVIRTUALLAYERS]; //for every layer we can have an execution queue
	f32 m_arrAdditiveWeights[numVIRTUALLAYERS];
	f32 m_arrLayerSpeedMultiplier[numVIRTUALLAYERS];
	f32 m_arrLayerBlending[numVIRTUALLAYERS];
	f32 m_arrLayerBlendingTime[numVIRTUALLAYERS];
	f32 m_arrLayerBlendingMult[numVIRTUALLAYERS];
	int32 m_arrStopAnimationQueue[numVIRTUALLAYERS]; // used on spu as a queue to dereff stopAnimationCalls

	int (*m_pEventCallback)(ICharacterInstance*,void*);
	void* m_pEventCallbackData;
	void SetEventCallback(CallBackFuncType func, void *pdata)
	{
		m_pEventCallback = func;
		m_pEventCallbackData = pdata;
	}
	AnimEventInstance GetLastAnimEvent() { return m_LastAnimEvent; };

	void AnimCallback ( CAnimation *arrAFIFO, uint32 AnimNo, uint32 AnimQueueLen);


	int (*m_pPreProcessCallback)(ICharacterInstance*,void*);
	void* m_pPreProcessCallbackData;
	void SetPreProcessCallback(CallBackFuncType func, void *pdata)
	{
		m_pPreProcessCallback = func;
		m_pPreProcessCallbackData = pdata;
	}

	AnimEventInstance m_LastAnimEvent;

	uint32 m_IsAnimPlaying;

	uint8 m_AnimationDrivenMotion;
	uint8 m_ShowDebugText;
	uint16 m_MirrorAnimation;
	uint16 m_ActiveLayer;

	void SetDebugging( uint32 debugFlags );

	bool m_CharEditMode: 1; //this is a flag register
	bool m_TrackViewExclusive : 1;
	uint32 m_bReinitializeAnimGraph : 1;

	
	Vec3 GetCurrentVelocity() {	return m_pSkeletonPose->GetLocator().m_velocity; };
	Vec3 GetCurrentAimDirection();
	Vec3 GetCurrentLookDirection();
	f32 GetCurrentSlope() { return m_pSkeletonPose->GetLocator().m_slope; };
	const QuatT& GetRelMovement();
	f32 GetRelRotationZ();
	f32 GetUserData(int i){ return m_fUserData[i]; }
	void InitSkeletonAnim( CCharInstance* pInstance, CSkeletonPose* pSkeletonPose );
	void ProcessAnimationUpdate(const QuatT rPhysLocationCurr, const QuatTS rAnimLocationCurr);
	void SyncProcessAnimations();


#ifdef PS3
	//need to copy parameters to execute in asynchronously
	void ProcessForwardKinematics( const QuatT rPhysLocationCurr, const QuatTS rAnimCharLocationCurr );
#else
	void ProcessForwardKinematics( const QuatT& rPhysLocationCurr, const QuatTS& rAnimCharLocationCurr );
#endif




	void ProcessForwardKinematics_stage3(const QuatT& rPhysLocationCurr, const QuatTS& rAnimLocationCurr);


	void FinishAnimationComputations();
	void FinishAnimationComputationsExecute();
	void SyncData();

	void SetPostProcessParameter( const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment*	pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 nOnRender );

	// since CGA animations are not yet supported on spu, we need to store the parameter to execute forwardkinematics for these later in the frame
	QuatT m_CGAPhysLocationCurr;
	QuatTS m_CGAAnimLocationCurr;

	f32	m_fAllowMultilayerAnim;
	f32 m_fUserData[NUM_ANIMATION_USER_DATA_SLOTS];

	CSkeletonPose* m_pSkeletonPose;
	CCharInstance* m_pInstance;


	bool					m_bDidRun;
	bool					m_bNeedPostProcess;
	bool					m_bSetDefaultPose;

	QuatT					m_PostProcessPhysLocationNext;
	QuatTS				m_PostProcessAnimLocationNext;
	IAttachment*	m_pPostProcessIAttachment;
	float					m_fPostProcessZoomAdjustedDistanceFromCamera;
	uint32				m_nPostProcessOnRender;

	uint32 nStopAnimationQueuePos;

	AnimEventInstance spuEventQueue[5];
	uint32 m_nEventQueuePos;
	QuatT m_RelativeMovement;

	bool m_hasInvalidController;
	const char *m_pWrongControllerName;

#if defined(PS3) && !defined(__SPU__) && !defined(__CRYCG__)
	volatile NSPU::NDriver::SExtJobState m_JobStateForwardKinematics;
	volatile NSPU::NDriver::SExtJobState m_JobStateProcessAnimations;
#else // provide padding to ensure the same layout
	char pad1[16] _ALIGN(16); 
	char pad2[16] _ALIGN(16);
#endif

	CSkeletonAnimTask m_threadTask;

	void PushLayer(IAnimationLayer* pLayer);

	DynArray<uint16>	m_arrJointMask; //limited to 16 layers

	uint8 GetJointMask(int32 nJointNo, uint8 layer);
	void SetJointMask(int32 nJointNo, uint8 layer, uint8 val);
	//enable & disable animation for one specified bone and layer
	bool SetJointMask(const char* szBoneName, uint32 nLayerNo, uint8 nVal);
	//enable & disable animation for one full layer
	bool SetLayerMask(uint32 nLayerNo, uint8 nVal);


private:
	void RemoveFirstAnimationFromQueue( int32 nLayer );
	void AppendAnimationToQueue( int32 nLayer, const CAnimation &rAnim );

	uint32 EvaluateTransitionFlags( CAnimation *arrAnimFiFo, uint32 numAnims );
	void TransitionsBetweenSeveralAnimations( CAnimation *arrAnimFiFo, uint32 numAnims );
	void UpdateTransitionTime( CAnimation *arrAnimFiFo, uint32 numAnims, f32 fDeltaTime );
	void AdjustTransitionWeights( CAnimation *arrAnimFiFo, uint32 numAnims );
	void LMGBlendWeightCalculation( CAnimation *arrAnimFiFo, uint32 numAnims );
	void ComputeBlendWeights( CAnimation *arrAnimFiFo, uint32 numAnims, f32 fDeltaTime, uint32 nLayer );
	void AdjustAnimationTimeForTimeWarpedAnimations( CAnimation *arrAnimFiFo, uint32 numAnims );	
	void UpdateAnimTimeAndExecBuffer( CAnimation *arrAnimFiFo, uint32 numAnims, uint32 nLayer, uint32 NumAnimsInQueue );
	
	void InitGlobalSPUVars();

private:
	void Commands_Create(const QuatT& rPhysLocationCurr, const QuatTS& rAnimLocationCurr, Command::CBuffer& buffer);
	void Commands_BasePlayback(const CAnimation& rAnim, f32& fRootPlayback, Command::CBuffer& buffer);
	void Commands_BaseEvaluationLMG(const CAnimation& rAnim, uint32 nTargetBuffer, Command::CBuffer& buffer);
	void Commands_LPlayback(const CAnimation& rAnim, uint32 nTargetBuffer, uint32 nVLayer, uint32 mask, Command::CBuffer& buffer);
	void Commands_CreateAimPoseQueue_New(uint32 numAnimsL0, uint32 nAimIKLayer, Command::CBuffer& buffer);
	void Commands_CreateAimPoseQueue_Old(uint32 numAnimsL0, uint32 nAimIKLayer, Command::CBuffer& buffer);

} _ALIGN(128);

#endif // _SKELETON_ANIM_H
