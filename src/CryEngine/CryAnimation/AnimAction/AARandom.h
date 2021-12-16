#ifndef AARandom_h
#define AARandom_h

#include <CryExtension/Impl/ClassWeaver.h>
#include <IAnimAction.h>

class CAnimActionActor;

class CAARandom :
	public IAnimAction
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimAction)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CAARandom, "AnimAction_Random", 0x669fa4812729418b, 0xad3b505e70f8ae59)

public:
	void Init( IAnimActionActor* pActor );
	
	void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	
	void* GetCAnimAction() const 
	{ 
		return (void*)this; 
	};

	virtual bool Start( const SAnimActionStartParams &params );
	virtual bool Stop( bool bAbort );

	int GetRandomAnimationIndex() const;
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
	int m_nNextAnimIndex;
	CAnimActionActor* m_pActor;
	ISkeletonAnim *m_pISkeletonAnim;
	ISkeletonPose *m_pISkeletonPose;

	// IAnimAction
protected:
	virtual void AnimActionInit() 
	{ 
		m_nCurrentAnimToken=0;
		m_nCurrentAnimIndex=-1;
		m_nNextAnimIndex=-1;
	}
	virtual const char* GetAAClassName() const { return GetPrototype()->GetAAClassName(); }
	virtual bool Update( const SPlaybackContext& context );
	virtual void DebugDraw( SDebugDrawInfo &info );
};

class AAClass_CRandom : public TAnimActionClass<AAClass_CRandom>
{
public:
	virtual const char* GetAAClassName() const { return "Random"; }


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

#endif // AARandom_h
