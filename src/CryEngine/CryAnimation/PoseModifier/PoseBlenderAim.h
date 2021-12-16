#ifndef PoseBlenderAim_h
#define PoseBlenderAim_h

#include <CryExtension/Impl/ClassWeaver.h>



//#define AimTRIANGLES (8)
//#define AimTRIANGLES (32)
#define AimTRIANGLES (72)

struct SAimIK
{
	int8	m_UseAimIK;
	int8	m_numActiveAimPoses;		//active animation in the FIFO queue
	int8	m_numActiveAnims2;		//active animation in the FIFO queue
	int8  m_nLayer;
	int16  m_nLastValidAimPose;
	int16  m_nArms;
	int16  m_nPose;

	SAimInfo m_AimInfo[MAX_EXEC_QUEUE*2]; 

	Vec3	m_AimIKTarget;
	Vec3	m_AimIKTargetSmooth;
	Vec3	m_AimIKTargetRate;
	f32	  m_AimIKTargetSmoothTime;
	Vec3	m_AimDirection;			//just for debugging

	f32 m_AimIKBlend;
	f32 m_AimIKInfluence;

	uint16 m_AimIKFadeout;
	uint16 m_AimIKFadeoutFlag;
	f32		 m_fAimIKFadeInTime;
	f32		 m_fAimIKFadeOutTime;
	Vec2	 m_PolarCoordinates;
	Vec2	 m_PolarCoordinatesSmooth;
	Vec2	 m_PolarCoordinatesRate;

	// Aim Reference Pose
	bool	m_applyAimRefPose;		// Whether to use aim reference pose feature
	DynArray<QuatT> m_refPoseRelQuats; // Relative quaternions of the reference pose
	QuatT	m_lHandOffset;				// Left hand bone offset relative to the absolute position of the right hand bone (in the reference pose)
	Quat	m_rUpperArmDelta;			// Upper arm orientation delta between the reference pose and base aim pose
	Quat	m_rForeArmDelta;			// Forearm orientation delta between the reference pose and base aim pose
	Quat	m_rHandDelta;					// Hand bone orientation delta between the reference pose and base aim pose

	// Double buffering
	int8 m_Set_UseAimIK;
	int16 m_Set_nArms;
	int32 m_Set_nLayer;
	Vec3 m_Set_AimIKTarget;
	f32 m_Set_AimIKTargetSmoothTime;
	f32 m_Set_AimIKBlend;
	uint16 m_Set_AimIKFadeoutFlag;
	f32 m_Set_fAimIKFadeInTime;
	f32 m_Set_fAimIKFadeOutTime;
	Vec2 m_Set_PolarCoordinates;

	SAimIK() { Init(); }

	void Init()
	{
		m_Set_UseAimIK=0;

		m_numActiveAimPoses=0;
		m_nLastValidAimPose=-1;
		for (uint32 i=0; i<MAX_EXEC_QUEUE*2; i++)
		{
			m_AimInfo[i].m_fWeight				= 0; 
			m_AimInfo[i].m_numAimPoses		= 0; 
			m_AimInfo[i].m_nGlobalAimID0	= -1; 
			m_AimInfo[i].m_nGlobalAimID1	= -1;
			m_AimInfo[i].m_fAnimTime			= 0.0f;
		}

		m_Set_nArms							=	3;
		m_nPose							=	0;
		m_Set_nLayer            = 16;
		m_Set_AimIKTarget				=	Vec3(ZERO);
		m_AimIKTargetSmooth	=	Vec3(ZERO);
		m_AimIKTargetRate		=	Vec3(ZERO);
		m_Set_AimIKTargetSmoothTime = 0.10f;
		m_AimDirection			=	Vec3(0,1,0);		//just for debugging

		m_Set_AimIKBlend				=0.0f; //percentage value between animation and aim-ik
		m_AimIKInfluence		=0.0f;

		m_AimIKFadeout			=0;
		m_Set_AimIKFadeoutFlag	=1;
		m_Set_fAimIKFadeOutTime = 1.0f/0.6f;  // = 1.0f/TransitionTime
		m_Set_fAimIKFadeInTime = 1.0f/0.3f;  // = 1.0f/TransitionTime
		m_Set_PolarCoordinates				= Vec2(0,0);

		m_applyAimRefPose = false;
		m_lHandOffset.SetIdentity();
		m_rUpperArmDelta.SetIdentity();
		m_rForeArmDelta.SetIdentity();
		m_rHandDelta.SetIdentity();

		m_UseAimIK = m_Set_UseAimIK;
		m_nArms = m_Set_nArms;
		m_nLayer = m_Set_nLayer;
		m_AimIKTarget = m_Set_AimIKTarget;
		m_AimIKTargetSmoothTime = m_Set_AimIKTargetSmoothTime;
		m_AimIKBlend = m_Set_AimIKBlend;
		m_AimIKFadeoutFlag = m_Set_AimIKFadeoutFlag;
		m_fAimIKFadeInTime = m_Set_fAimIKFadeInTime;
		m_fAimIKFadeOutTime = m_Set_fAimIKFadeOutTime;
		m_PolarCoordinates				= m_Set_PolarCoordinates;
		m_PolarCoordinatesSmooth	= m_Set_PolarCoordinates;
		m_PolarCoordinatesRate		= m_Set_PolarCoordinates;
	}
};

class CPoseBlenderAim :	
	public IAnimationPoseBlenderAim,
	public SAimIK
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
		CRYINTERFACE_ADD(IAnimationPoseBlenderAim)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CPoseBlenderAim, "AnimationPoseModifier_PoseBlenderAim", 0x058c3e18b9574faf, 0x8989b9cb2cff0d64)

public:
	void UpdateParameters(const SAnimationPoseModiferParams& params);

private:
	uint32 BuildLocalAimPoseArray(const SAnimationPoseModiferParams& params, AimIKPose* parrAimIKPoses);
	float BuildLocalAimPoseArrayOtherLayer( AimIKPose* parrAimIKPoses, SAimInfo& rAimInfo, uint32 numRotJoints, uint32 numPosJoints);

	void BlendAimPoses(const SAnimationPoseModiferParams& params, IPol& ipol, AimIKPose* arrAimIKPoses, f32 fIKBlend);
	SAimDirection EvaluatePose(const SAnimationPoseModiferParams& params, uint32 aimpose, AimIKPose* parrAimIKPoses );
	IPol CheckTriangles(const SAnimationPoseModiferParams& params, uint32 k0,uint32 k1,uint32 k2,ColorB rgb, SAimDirection* ls,const Vec3& localAimTarget, AimIKPose* parrAimIKPoses, f32 fIKBlend );
	void BlendAimPoses_Additive(const SAnimationPoseModiferParams& params, IPol &ipol, int32 j,const Quat& aim0,const Quat& aim1,const Quat& aim2,const Quat& rel, f32 fIKBlend);
	void BlendAimPoses_Normal(const SAnimationPoseModiferParams& params, IPol &ipol, int32 j,const Quat& aim0,const Quat& aim1,const Quat& aim2,f32 fIKBlend);
	void BlendAimPoses_Normal(const SAnimationPoseModiferParams& params, IPol &ipol, int32 j,const Vec3& aim0,const Vec3& aim1,const Vec3& aim2,f32 fIKBlend);

	// This interface
public:
	void SetTarget(const Vec3& target) { if (target.IsValid()) m_Set_AimIKTarget = target; }
	void SetArms(uint32 arms) { m_Set_nArms = arms; }
	void SetBlendTime(f32 time) { if (time != -1.0f) m_Set_AimIKBlend = time; }
	f32 GetBlendTime() { return m_Set_AimIKBlend; }
	void SetFadeOut(uint32 a) { m_Set_AimIKFadeoutFlag = a > 0; }
	void SetFadeOutSpeed(f32 time) { m_Set_fAimIKFadeOutTime = 1.0f/time; }
	void SetFadeInSpeed(f32 time) { m_Set_fAimIKFadeInTime = 1.0f/time; }
	void SetTargetSmoothTime(f32 time) { m_Set_AimIKTargetSmoothTime = time; }
	uint32 GetStatus() { return uint32(m_Set_UseAimIK && (m_AimIKInfluence > 0.9f)); }
	f32 GetBlend() { return m_AimIKInfluence; }
	void SetPolarCoordinates(const Vec2& pc) 
	{
		m_Set_PolarCoordinates = pc;	
	};

	void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	// IAnimationPoseModifier
public:
	virtual bool ExecutePreProcess();
	virtual bool Execute(const SAnimationPoseModiferParams& params);

#if (NEWAIMSYSTEM)
	virtual bool ExecuteAimIK_New(const SAnimationPoseModiferParams& params);
	void AccumulateAimPoses( CCharInstance* pInstance,GlobalAnimationHeaderAIM& rGAH,QuatT* arrRelPose,f32 fIKBlend,const Vec3& rAimTarget,int32 widx);
	void DebugVEGrid(CCharInstance* pInstance, SAimInfo& rAimInfo,f32 fIKBlend,int32 widx);
#endif

};

#endif // PoseBlenderAim_h
