#ifndef __WEAPONFP_AIMING_H__
#define __WEAPONFP_AIMING_H__

#include "ItemDefinitions.h"
#include "ItemSharedParams.h"

struct SSTAP_CVars
{
	SSTAP_CVars()
		: m_initialized(false)
	{

	}

	~SSTAP_CVars()
	{
		ReleaseCVars();
	}

	void InitCvars();

	float STAP_MF_All;
	float STAP_MF_Scope;
	float STAP_MF_ScopeVert;
	float STAP_MF_HeavyWeapon;
	float STAP_MF_Up;
	float STAP_MF_Down;
	float STAP_MF_Left;
	float STAP_MF_Right;
	float STAP_MF_Front;
	float STAP_MF_Back;
	float STAP_MF_StrafeLeft;
	float STAP_MF_StrafeRight;
	float STAP_MF_VertMotion;
	float STAP_MF_VelFactorVert;
	float STAP_MF_VelFactorHoriz;

private:

	void ReleaseCVars();

	bool  m_initialized;
};

struct SData_WeaponFPAiming
{
	SData_WeaponFPAiming()
		: 
		transitionTime(0.175f),
		layer(0),
		stopOnExit(1),
		exitTime(0.2f),
		entryTime(0.25f),
		easeFactorInc(5.25f),
		easeFactorDec(10.0f),
		strafeScopeFactor(0.0f),
		rotateScopeFactor(5.0f)
	{
		for (int i=0; i<WeaponAimAnim::Total; i++)
		{
			blendFactors[i] = 1.0f;
		}
		//--- Disable broken idle for now (export name fail!)
		//blendFactors[WeaponAimAnim::Idle] = 0.0f;
	}

	int		layer;
	int		stopOnExit;
	float	transitionTime;
	float	entryTime;
	float	exitTime;
	float	easeFactorInc;
	float	easeFactorDec;
	float	strafeScopeFactor;
	float	rotateScopeFactor;
	float blendFactors[WeaponAimAnim::Total];
};

struct SParams_WeaponFPAiming
{
	SParams_WeaponFPAiming() 
		: 
		skelAnim(NULL),
		characterInst(NULL),
		aimDirection(ZERO),
		velocity(ZERO),
		position(ZERO),
		inputRot(ZERO),
		inputMove(ZERO),
		superJump(false),
		groundDistance(0.0f),
		isZoomed(false),
		isHeavyWeapon(false),
		animCache(0, 0, eNanoSuitMode_Invalid, 0),
		transitionTime(-1.0f),
		overlayFactor(1.0f),
		blendBasePose(true),
		runToSprintBlendTime(0.2f),
		sprintToRunBlendTime(0.2f)
	{
		memset(animID, -1, sizeof(animID));
	}

	void SetAnimation(int layerId, int animationId);
	uint32 UpdateStatus(const SCachedItemAnimation &newAnimCache);

	int				animID[WeaponAimAnim::Total];
	struct ISkeletonAnim  *skelAnim;
	struct ICharacterInstance *characterInst;
	Ang3			inputRot;
	Vec3			inputMove;
	Vec3			aimDirection;
	Vec3			velocity;
	Vec3			position;
	float			groundDistance;
	float			transitionTime;
	float			runToSprintBlendTime;
	float			sprintToRunBlendTime;
	float			overlayFactor;
	bool			superJump;
	bool			isOnGround;
	bool			isZoomed;
	bool			isHeavyWeapon;
	bool			isSprinting;
	bool			blendBasePose;

private:
	SCachedItemAnimation animCache;
};


class CWeaponFPAiming
{
public:

	CWeaponFPAiming();

	void Update(const SParams_WeaponFPAiming &params);
	void SetActive(bool active);
	void SnapSwitch(bool snap);
	float GetRunFactor() const { return m_runFactor; }

	float GetHorizontalSweay() const {return m_interpHoriz;}
	float GetVerticalSweay() const {return m_interpVert;}
	float GetSprintFactor() const {return m_sprintFactor;}

private:

	struct AnimInstall
	{
		WeaponAimAnim::Type anim;
		float				factor;

		AnimInstall()
		{
			anim	= WeaponAimAnim::Total;
			factor  = 1.0f;
		}
	};

	void Start(ISkeletonAnim *skelAnim);
	void Stop(ISkeletonAnim *skelAnim);

	int   m_currentAnimID[WeaponAnimLayerID::Total];
	float m_interpVert;
	float m_interpHoriz;
	float m_initialTime;
	float m_fallFromHeight;
	float m_runFactor;
	float m_sprintFactor;
	float m_translationPinFactor;
	bool  m_falling;
	bool  m_enabled;
	bool  m_pendingEnabled;
	bool  m_firstUpdate;
	bool  m_snapSwitch;
	Vec3  m_smoothedVelocity;
	Vec3  m_lastVelocity;
	float m_interpFront;
	float m_interpSide;

	SData_WeaponFPAiming m_data;

	static SSTAP_CVars s_stapCVars;

};

#endif //__WEAPONFP_AIMING_H__
