#ifndef FeetLock_h
#define FeetLock_h

struct SFeetData
{
	QuatT m_lAnkle, m_rAnkle;
	Vec3 m_lCross, m_rCross;
	Vec3 m_lKneeDir, m_rKneeDir;
	bool m_IsAnkleValid;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

class CFeetPoseStore :
	public IAnimationPoseModifier
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CFeetPoseStore, "AnimationPoseModifier_FeetPoseStore", 0x4095cfb096b5494f, 0x864d3c007b71d31d)

private:
	void ComputeFootPositions(QuatT &rRelativeCalf, QuatT &rRelativeTight, QuatT &rFoot, QuatT &plevis, Vec3 &rCross, Vec3 &rKneeDir, QuatT &rAnkle);

	// IAnimationPoseModifier
public:
	virtual bool Execute(const SAnimationPoseModiferParams& params);

	void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pFeetData);
	}
public://private:
	SFeetData* m_pFeetData;
};

class CFeetPoseRestore :
	public IAnimationPoseModifier
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CFeetPoseRestore, "AnimationPoseModifier_FeetPoseRestore", 0x90662f0ed05a4bf4, 0x8fb69924b5da2872)

	// IAnimationPoseModifier
public:
	virtual bool Execute(const SAnimationPoseModiferParams& params);

	void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pFeetData);
	}
public://private:
	SFeetData* m_pFeetData;
};

class CFeetLock
{
private:
	IAnimationPoseModifierPtr m_store;
	IAnimationPoseModifierPtr m_restore;

public:
	CFeetLock();

public:
	void Reset(){m_feetData.m_lKneeDir=Vec3(ZERO); m_feetData.m_rKneeDir=Vec3(ZERO);}

	IAnimationPoseModifier* Store() { return m_store.get(); }
	IAnimationPoseModifier* Restore() { return m_restore.get(); }

private:
	SFeetData m_feetData;
};

#endif // FeetLock_h
