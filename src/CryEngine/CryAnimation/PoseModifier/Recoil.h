#ifndef Recoil_h
#define Recoil_h

namespace PoseModifier {

class CRecoil : public IAnimationPoseModifier
{
public:
	struct State
	{
		f32 time;
		f32 duration;
		f32 strengh;
		f32 kickin;
		uint32 arms;

		State()
		{
			Reset();
		}

		void Reset()
		{
			time = 100.0f;
			duration = 0.0f;
			strengh = 0.0f;
			kickin = 0.8f;
			arms = 3;
		}
	};

public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CRecoil, "AnimationPoseModifier_Recoil", 0xd7900cb9e7be4825, 0x99e1cc1211f9c561)

public:
	void SetState(const State& state) { m_state = state; m_bStateUpdate = true; }

private:
	f32 RecoilEffect(f32 t); 

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModiferParams& params);
	virtual bool Execute(const SAnimationPoseModiferParams& params);
	virtual void Synchronize();

	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

private:
	State m_state;
	State m_stateExecute;
	bool m_bStateUpdate;
} _ALIGN(32);

} // namespace PoseModifier

#endif // Recoil_h
