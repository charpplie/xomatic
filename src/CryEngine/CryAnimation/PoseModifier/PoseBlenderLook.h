#ifndef PoseBlenderLook_h
#define PoseBlenderLook_h

class CAttachment;

class CPoseBlenderLook : public IAnimationPoseBlenderDir
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
		CRYINTERFACE_ADD(IAnimationPoseBlenderDir)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CPoseBlenderLook, "AnimationPoseModifier_PoseBlenderLook", 0x058c3e18b9874faf, 0x8989b9cb2cff0d64)

		// This interface
public:
	void SetState(bool state) { m_blender.m_Set_UseDirIK = state; }
	void SetTarget(const Vec3& target) { if (target.IsValid()) m_blender.m_Set_vDirIKTarget = target; }
	void SetLayer(uint32 layer) { m_blender.m_Set_nLayer = MAX(layer,1);	}
	void SetBlendTime(f32 time) { if (time != -1.0f) m_blender.m_Set_fDirIKBlend = time; }
	f32 GetBlendTime() { return m_blender.m_Set_fDirIKBlend; }
	void SetFadeoutAngle(f32 a) { m_blender.m_Set_fDirIKFadeoutAngle = a > 0; }
	void SetFadeOutTime(f32 time) { m_blender.m_Set_fDirIKFadeOutTime = time; }
	void SetFadeInTime(f32 time) { m_blender.m_Set_fDirIKFadeInTime = time; }
	void SetTargetSmoothTime(f32 time) { m_blender.m_Set_fDirIKTargetSmoothTime = time; }
	uint32 GetStatus() { return uint32(m_blender.m_Set_UseDirIK && (m_blender.m_fDirIKInfluence > 0.9f)); }
	f32 GetBlend() { return m_blender.m_fDirIKInfluence; }
	void SetPolarCoordinates(const Vec2& pc) 
	{
		m_blender.m_Set_PolarCoordinates = pc;	
	};

public:
	virtual bool Prepare(const SAnimationPoseModiferParams& params);
	virtual bool Execute(const SAnimationPoseModiferParams& params);
	virtual void Synchronize();

	bool Update(const SAnimationPoseModiferParams& params);

	void GetMemoryUsage(ICrySizer* pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}

public:

	CAttachment* m_pAttachmentEyeLeft;
	CAttachment* m_pAttachmentEyeRight;
	QuatT m_ql;
	QuatT m_qr;
	int32 m_EyeIdxL;
	int32 m_EyeIdxR;

	Quat m_additionalRotationLeft;
	Quat m_additionalRotationRight;

	SDirectionalBlender m_blender;
};


#endif // LookAt_h
