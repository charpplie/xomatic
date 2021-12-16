#ifndef AALocomotion_h
#define AALocomotion_h

#include <CryExtension/Impl/ClassWeaver.h>
#include <IAnimAction.h>

#include "AnimActionActor.h"

class CryCharManager;

class CAALocomotion :
	public IAnimAction,
	public IAALocomotion
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimAction)
		CRYINTERFACE_ADD(IAALocomotion)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CAALocomotion, "AnimAction_Locomotion", 0xf8e0feedb3af450f, 0x82064288becb4192)

public:
	void Init( IAnimActionActor* pActor );

	void GetMemoryUsage(ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
	}
protected:
	bool StateMachine( CAnimation* pAnimation, f32 fDesiredSpeed );
	void StandingAnims( CAnimation* pAnimation, f32 fDesiredSpeed, const char* strIdle, const char* strIdle2Move, f32 yaw_radiant );
	void Locomotion( CAnimation* pAnimation, f32 fDesiredSpeed, const char* strIdle, const char* strIdle2Move, const char* strWalk,const char* strRun, uint32 STATE_MOVE, uint32 STATE_IDLE2MOVE );
	void SnapTurn( f32 fDesiredSpeed, f32 fLocomotionWeight, const Vec2& vWorldDesiredBodyDirection, const Vec2& vWorldCurrentBodyDirection  );
	f32 Debug_Substate_Transitions( f32 fDesiredSpeedSmooth );

	enum
	{
		STATE_STAND,
		STATE_WALK,
		STATE_RUN,
		STATE_IDLE2WALK,
		STATE_IDLE2RUN,
		STATE_SNAPTURN
	};

	CAnimActionActor* m_pActor;
	ISkeletonAnim* m_pISkeletonAnim;
	IAnimationSet* m_pIAnimationSet;

	CryCharAnimationParams m_Qparams;

	f32 m_fDesiredSpeed;
	f32 m_fGroundSlopeMoveDir;
	f32 m_fGroundSlope;
	Vec2 m_vWorldDesiredBodyDirection;
	Vec2 m_vWorldDesiredMoveDirection;


	f32 m_fGamePadNoise;
	f32 m_fGamePadNoiseSmooth;
	f32 m_fGamePadNoiseSmoothRate;
	f32 m_fScaleNoise;

	f32 m_fDesiredTurnSpeedOld;
	f32 m_fDesiredTurnSpeedSmooth;
	f32 m_fDesiredTurnSpeedSmoothRate;
	f32 m_arrDesiredTurnSpeed[0x40];

	f32 m_fNaturalSpeedSmooth;
	f32 m_fNaturalSpeedSmoothRate;

	uint32 m_keyrcr_SPEED;

	// IAnimAction
protected:
	virtual void AnimActionInit() { }
	virtual const char* GetAAClassName() const { return GetPrototype()->GetAAClassName(); }
	virtual bool Update( const SPlaybackContext& context );
	virtual void DebugDraw( SDebugDrawInfo &info );

	// IAALocomotion
protected:
	virtual f32 GetDesiredSpeed() const { return m_fDesiredSpeed; }
	virtual const Vec2& GetWorldDesiredBodyDirection() const { return m_vWorldDesiredBodyDirection; }
	virtual const Vec2& GetWorldDesiredMoveDirection() const { return m_vWorldDesiredMoveDirection; }
	virtual const f32 GetDesiredSlope() const { return m_fGroundSlope; }
	virtual const f32 GetDesiredSlopeMoveDir() const { return m_fGroundSlopeMoveDir; }
	virtual void SetDesiredSpeed(f32 fDesiredSpeed)	{ m_fDesiredSpeed=fDesiredSpeed; }
	virtual void SetDesiredSlope(f32 fDesiredSlopeMoveDir,f32 fDesiredSlope) { m_fGroundSlopeMoveDir=fDesiredSlopeMoveDir;  m_fGroundSlope=fDesiredSlope; }
	virtual void SetWorldDesiredBodyDirection(const Vec3& vWorldDesiredBodyDirection) { m_vWorldDesiredBodyDirection=Vec2(vWorldDesiredBodyDirection); }
	virtual void SetWorldDesiredMoveDirection(const Vec3& vWorldDesiredMoveDirection) { m_vWorldDesiredMoveDirection=Vec2(vWorldDesiredMoveDirection); }
};

class AAClass_CLocomotion : public TAnimActionClass<AAClass_CLocomotion>
{
public:
	virtual const char* GetAAClassName() const { return "Locomotion"; }

	enum EProperties
	{
		EP_Idle,
		EP_Walk,
		EP_Run,
		EP_Idle2Walk,
		EP_Idle2Run,
		EP_RunStop,
		EP_WalkStop,

		EP_SharpTransitionRight90,
		EP_SharpStartRight90,
		EP_SharpTransitionRight180,
		EP_SharpStartRight180,

		EP_SharpTransitionLeft90,
		EP_SharpStartLeft90,
		EP_SharpTransitionLeft180,
		EP_SharpStartLeft180,

		EP_Walk2Run_Threshold,
		EP_Run2Walk_Threshold,
		EP_TurnSpeed,
	};

	virtual const SAnimActionPropertyDesc* GetPropertiesDesc() const
	{
		static const SAnimActionPropertyDesc propertyDesc[] =
		{
			{ eAAPT_string, "Idle",				"WrongName" },
			{ eAAPT_string, "Walk",				"WrongName" },
			{ eAAPT_string, "Run",				"WrongName" },
			{ eAAPT_string, "Idle2Walk",	"WrongName" },
			{ eAAPT_string, "Idle2Run",		"WrongName" },
			{ eAAPT_string, "RunStop",		"WrongName" },
			{ eAAPT_string, "WalkStop",		"WrongName" },

			{ eAAPT_string, "SharpTransitionRight90","WrongName" },
			{ eAAPT_string, "SharpStartRight90","WrongName" },
			{ eAAPT_string, "SharpTransitionRight180","WrongName" },
			{ eAAPT_string, "SharpStartRight180","WrongName" },

			{ eAAPT_string, "SharpTransitionLeft90","WrongName" },
			{ eAAPT_string, "SharpStartLeft90","WrongName" },
			{ eAAPT_string, "SharpTransitionLeft180","WrongName" },
			{ eAAPT_string, "SharpStartLeft180","WrongName" },

			{ eAAPT_float, "Walk2Run_Threshold", "2.0001" },
			{ eAAPT_float, "Run2Walk_Threshold", "1.6" },
			{ eAAPT_float, "TurnSpeed", "1.1" },
			{ eAAPT_null }
		};
		return propertyDesc;
	}
};

#endif // AALocomotion_h
