#ifndef LookAt_h
#define LookAt_h

#define LOOKIK_BLEND_RATIOS 5
struct SLookIK 
{
	Quat m_oldSpine1;
	Quat m_oldSpine2;
	Quat m_oldSpine3;
	Quat m_oldNeck;
	Quat m_oldNeck1;
	Quat m_oldHead;
	Quat m_oldEyeLeft;
	Quat m_oldEyeRight;
	
	Vec3 m_LookDirection;		//just for debugging

	Vec3 m_LookIKTarget;
	Vec3 m_LookIKTargetSmooth;
	Vec3 m_LookIKTargetSmoothRate;

	f32 m_LookIKBlend;
	f32 m_LookIKFOR;

	f32 m_lookIKBlends[LOOKIK_BLEND_RATIOS];
	f32 m_lookIKBlendsSmooth[LOOKIK_BLEND_RATIOS];
	f32 m_lookIKBlendsSmoothRate[LOOKIK_BLEND_RATIOS];
			
	uint16 m_LookIKFadeout;
	
	uint8 m_UseLookIK;
	uint8 m_AllowAdditionalTransforms;

	Quat	m_addHead; 
	Quat	m_addLEye; 
	Quat	m_addREye; 
		
	SLookIK() {	Init();	};

	void Init() 
	{
		m_UseLookIK=0;
		m_AllowAdditionalTransforms=1;
		m_LookIKFadeout=0;

		m_LookDirection=Vec3(0,1,0);	//just for debugging
		m_LookIKTarget=Vec3(ZERO);
		m_LookIKTargetSmooth=Vec3(ZERO);
		m_LookIKTargetSmoothRate=Vec3(ZERO);

		m_LookIKBlend	=0.0f;
		m_LookIKFOR		=gf_PI*0.5f;


		m_lookIKBlends[0] = 0.04f;
		m_lookIKBlends[1] = 0.06f;
		m_lookIKBlends[2] = 0.08f;
		m_lookIKBlends[3] = 0.15f;
		m_lookIKBlends[4] = 0.60f;

		m_lookIKBlendsSmooth[0] = 0.04f;
		m_lookIKBlendsSmooth[1] = 0.06f;
		m_lookIKBlendsSmooth[2] = 0.08f;
		m_lookIKBlendsSmooth[3] = 0.15f;
		m_lookIKBlendsSmooth[4] = 0.60f;
		m_lookIKBlendsSmoothRate[0] = 0.04f;
		m_lookIKBlendsSmoothRate[1] = 0.06f;
		m_lookIKBlendsSmoothRate[2] = 0.08f;
		m_lookIKBlendsSmoothRate[3] = 0.15f;
		m_lookIKBlendsSmoothRate[4] = 0.60f;

		m_oldSpine1.SetIdentity();
		m_oldSpine2.SetIdentity();
		m_oldSpine3.SetIdentity();
		m_oldNeck.SetIdentity();
		m_oldNeck1.SetIdentity();
		m_oldHead.SetIdentity();
		m_oldEyeLeft.SetIdentity();
		m_oldEyeRight.SetIdentity();
	};
	
} _ALIGN(32);

class CLookAt :
	public IAnimationPoseModifier,
	public SLookIK
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CLookAt, "AnimationPoseModifier_LookAt", 0xd40464299f4f47d3, 0xa5a8aa1a7ff5d365)

private:
	void ComputeNewSpineAndHeadOrientation(const SAnimationPoseModiferParams& params, SLookIK &rLookIK, QuatT* pRelativeQuatIK, Quat &relHeadQuat);
	void CalculateEyeLookDirection(const SAnimationPoseModiferParams& params, QuatT &ql, QuatT &qr, CAttachment* pAttachmentL, CAttachment* pAttachmentR, SLookIK &rLookIK, Vec3 &CamPos, QuatT* pRelativeQuatIK);

	bool ExecuteLocal(const SAnimationPoseModiferParams& params);
	// IAnimationPoseModifier
public:
	virtual bool Execute(const SAnimationPoseModiferParams& params);

	void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif // LookAt_h
