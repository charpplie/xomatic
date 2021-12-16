#ifndef __ACTOR_HIT_REACT_CONTROLLER__
#define __ACTOR_HIT_REACT_CONTROLLER__

class CActor;
struct HitInfo;

class ActorHitReactController
{
public:

	ActorHitReactController();
	~ActorHitReactController();

	void Initialise(CActor *actor);

	void OnKill(CActor *actor, const HitInfo* hitInfo);

	inline bool IsInitialised() const
	{
		return m_initialised;
	}

private:

	class HitReactType
	{
	public:

		enum Flags
		{
			OrientateToImpact		= (1<<0),
			RagdollOnCollision	= (1<<1),
			TriggerRagdoll			= (1<<2)
		};

		static const int MAX_LEN_HR_TYPE = 16;
		uint32 m_hitTypeMask;
		uint32 m_hitLimbMask;
		char m_type[MAX_LEN_HR_TYPE];
		uint32 m_flags;
		float m_minSpeed;
		int		m_numVariants;

		HitReactType();
		HitReactType(const char *type);

		void SetType(const char *type);
	};

	std::vector<HitReactType> m_options;
	bool m_initialised;
};

#endif __ACTOR_HIT_REACT_CONTROLLER__
