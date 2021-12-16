#ifndef AASequence_h
#define AASequence_h

#include <CryExtension/Impl/ClassWeaver.h>
#include <IAnimAction.h>

class CAnimActionActor;

class CAASequence :
	public IAnimAction
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimAction)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CAASequence, "AnimAction_Sequence", 0xd052d15da2824a15, 0x905ffd10522e9951)

public:
	void Init( IAnimActionActor* pActor );

	void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	virtual const char* GetAAClassName() const { return GetPrototype()->GetAAClassName(); }
	void* GetCAnimAction() const 
	{ 
		return (void*)this; 
	};
	virtual void AnimActionInit() 
	{ 
		m_nCurrentAnimToken=0;
		m_nCurrentAnimIndex=-1;
	}
	virtual bool Update( const SPlaybackContext& context );
	virtual bool Start( const SAnimActionStartParams &params );
	virtual bool Stop( bool bAbort );
	virtual void DebugDraw( SDebugDrawInfo &info );
	bool StartNextAnimation( const SAnimActionStartParams &params );

	Vec2 vdummy;
	virtual f32 GetDesiredSpeed() const { return 0; }
	virtual const Vec2& GetWorldDesiredBodyDirection() const { return vdummy; }
	virtual const Vec2& GetWorldDesiredMoveDirection() const { return vdummy; }
	virtual const f32		GetDesiredSlope() const { return 0; }
	virtual const f32		GetDesiredSlopeMoveDir() const { return 0; }
	virtual void SetDesiredSpeed(f32 fDesiredSpeed)	{ }
	virtual void SetDesiredSlope(f32 fDesiredSlopeMoveDir,f32 fDesiredSlope) {}
	virtual void SetWorldDesiredBodyDirection(const Vec3& vWorldDesiredBodyDirection) {}
	virtual void SetWorldDesiredMoveDirection(const Vec3& vWorldDesiredMoveDirection) {}

	int m_nCurrentAnimIndex;
	int m_nCurrentAnimToken;
	CAnimActionActor* m_pActor;
	ISkeletonAnim *m_pISkeletonAnim;
	ISkeletonPose *m_pISkeletonPose;
};

class AAClass_CSequence :  public TAnimActionClass<AAClass_CSequence>
{
public:
	virtual const char* GetAAClassName() const { return "Sequence"; }

	enum EProperties
	{
		EP_TransitionTime,
	};
	virtual const SAnimActionPropertyDesc* GetPropertiesDesc() const
	{
		static const SAnimActionPropertyDesc propertyDesc[] =
		{
			{ eAAPT_float, "TransitionTime", "0.2" },
			{ eAAPT_null }
		};
		return propertyDesc;
	}



	enum EChildProperties
	{
		ECP_RandomWeight,
		ECP_TransitionTime,
		ECP_AllowLookAt,
	};

	// returns an array of property descriptors for child animation entries (eAAPT_null terminated)
	virtual const SAnimActionPropertyDesc* GetChildPropertiesDesc() const
	{
		static const SAnimActionPropertyDesc propertyDesc[] =
		{
			{ eAAPT_float, "Random_Weight", "1.0" },
			{ eAAPT_float, "Trans_Time",  "0.0" },
			{ eAAPT_float, "Allow_LookAt",  "0.0" },
			{ eAAPT_null }
		};
		return propertyDesc;
	}
};

#endif // AASequence_h
