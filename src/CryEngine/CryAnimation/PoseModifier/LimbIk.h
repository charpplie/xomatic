#ifndef LimbIk_h
#define LimbIk_h

class CLimbIk :
	public IAnimationPoseModifier
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CLimbIk, "AnimationPoseModifier_LimbIk", 0x3b00bbad5b9c4fa4, 0x97e9b720fcbc8839)

private:
	struct Setup
	{
		bool adjustEndEffector;
		uint64 setup;
		QuatT targetPositionLocal;
	};

private:
	Setup m_setupsBuffer[2][16];

	Setup* m_pSetups;
	uint32 m_setupCount;

	Setup* m_pSetupsExecute;
	uint32 m_setupCountExecute;

public:
	void AddSetup(uint64 setup, const QuatT& targetPositionLocal, const bool adjustEndEff=false);

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModiferParams& params);
	virtual bool Execute(const SAnimationPoseModiferParams& params);
	virtual void Synchronize();

	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif // LimbIk_h
