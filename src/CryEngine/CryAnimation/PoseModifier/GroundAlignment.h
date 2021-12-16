#ifndef GroundAlignment_h
#define GroundAlignment_h

class CGroundAlignment :
	public IAnimationGroundAlignment
{
private:
	struct State
	{
		State() :
			rootHeight(0.0f),
			planeLeft(Vec3Constants<float>::fVec3_OneZ, 0.0f),
			planeRight(Vec3Constants<float>::fVec3_OneZ, 0.0f),
			alignSkeletonVertical(false),
			jointIndexLeft(-1),
			jointIndexRight(-1),
			jointIndexPelvis(-1)
		{
		}

		f32 rootHeight;
		Plane planeLeft;
		Plane planeRight;

		bool alignSkeletonVertical;

		int32 jointIndexLeft;
		int32 jointIndexRight;
		int32 jointIndexPelvis;
	};

public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
		CRYINTERFACE_ADD(IAnimationGroundAlignment)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CGroundAlignment, "AnimationPoseModifier_GroundAlignment", 0x6de1ac5816c94c48, 0x8fecf3ee1f6bdfab)

public:
	void SetData(const bool alignSkeletonVertical, const f32 rootHeight, const Plane& planeLeft, const Plane& planeRight, int32 jointIndexPelvis) 
	{
		m_state.alignSkeletonVertical = alignSkeletonVertical;
		m_state.rootHeight = rootHeight; 
		m_state.planeLeft = planeLeft; 
		m_state.planeRight = planeRight;
		m_state.jointIndexPelvis = jointIndexPelvis;
	}

private:
	uint32 SetFootGroundAlignment(const SAnimationPoseModiferParams& params, const Vec3& normal2, int32 CalfIdx, int32 FootIdx, int32 HeelIdx, int32 Toe0Idx);
	uint32 PerformGroundAlignmentCCD(const SAnimationPoseModiferParams& params, const char* strLEG, const Plane& GroundPlane);
	void MoveSkeletonVertical(const SAnimationPoseModiferParams& params, f32 vertical);

	bool LimbAlign(const SAnimationPoseModiferParams& params, uint32 index, const char* ik, const Plane& plane);

	bool ExecuteNew(const SAnimationPoseModiferParams& params);

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

	f32 m_LFootGroundAlign;
	f32 m_RFootGroundAlign;
};

#endif // GroundAlignment_h
