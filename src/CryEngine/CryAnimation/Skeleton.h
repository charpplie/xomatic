#ifndef Skeleton_h
#define Skeleton_h

class CModelSkeleton;

namespace Skeleton {

class CPoseData
{
public:
	bool Initialize(const CModelSkeleton& skeleton);
	void Initialize(const CPoseData& poseData);

	ILINE uint32 GetJointCount() const { return m_jointsRelative.size(); }
	ILINE const QuatT* GetJointsRelative() const { return &m_jointsRelative[0]; }
	ILINE const QuatT* GetJointsAbsolute() const { return &m_jointsAbsolute[0]; }
	ILINE const Status4* GetJointsStatus() const { return &m_jointsStatus[0]; }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_jointsRelative );
		pSizer->AddObject( m_jointsAbsolute );
		pSizer->AddObject( m_jointsStatus );
	}

public://private:
	DynArray<QuatT> m_jointsRelative;
	DynArray<QuatT> m_jointsAbsolute;
	DynArray<Status4> m_jointsStatus;
};

class CLocator
{
public:
	CLocator();
	~CLocator();

public:
	bool Initialize();

public://private:
	f32 m_speed;
	f32 m_turn;
	f32 m_strafe;
	f32 m_slope;

	Vec3 m_velocity;
	Vec3 m_translationRelative;
	f32 m_rotationRelative;
} _ALIGN(128);

}; // namespace Skeleton

#endif // Skeleton_h
