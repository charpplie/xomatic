#ifndef __SPOTTER_H__
#define __SPOTTER_H__

#pragma once


#include "Actor.h"
#include "GameUtils.h"
#include "Pinger_Mechanics.h"


struct IDebugHistoryManager;

struct SSpotterStats : public SActorStats
{
	SSpotterStats()
	{
		memset(this,0,sizeof(SSpotterStats));
	}
	void Serialize( TSerialize ser );

};


class CSpotter:
	public CActor
{
private:
	struct SSpotterModelInfo
	{
		Vec3		weaponOffset;
		Vec3		eyeOffset;

		Matrix33  viewMtx;  //Actual view direction
		Matrix33	baseMtx;	//base rotation matrix, rotation on the Z axis

		SSpotterModelInfo() :
			weaponOffset(ZERO), eyeOffset(ZERO)
		{
			baseMtx.SetIdentity();
			viewMtx.SetIdentity();
		}
	};

public:
	struct SBodyInfo
	{
		Vec3		vEyePos;
		Vec3		vEyeDir;
		Vec3		vEyeDirAnim;
		Vec3		vFwdDir;
		Vec3		vUpDir;
		Vec3		vFireDir;
		Vec3		vFirePos;
		float		maxSpeed;
		float		normalSpeed;
		float		minSpeed;
		EStance		stance;
		AABB		m_stanceSizeAABB;	// approximate local bounds of the stance.
		AABB		m_colliderSizeAABB;	// approximate local bounds of the stance collider only.

		SBodyInfo() : vEyePos(ZERO), vEyeDir(ZERO), vEyeDirAnim(ZERO),
			vFwdDir(ZERO), vUpDir(ZERO), vFireDir(ZERO),
			maxSpeed(0), normalSpeed(0), minSpeed(0),
			stance(STANCE_NULL)
		{
			m_stanceSizeAABB.min	=Vec3(ZERO);
			m_stanceSizeAABB.max	=Vec3(ZERO);
			m_colliderSizeAABB.min=Vec3(ZERO);
			m_colliderSizeAABB.max=Vec3(ZERO);
		}
	};

	struct SMovementRequestParams
	{
		bool	aimLook;
		int		bodystate;
		Vec3	vShootTargetPos;
		Vec3	vAimTargetPos;
		Vec3	vLookTargetPos;
		Vec3	vMoveDir;
		float	fDesiredSpeed;
		EActorTargetPhase	eActorTargetPhase;
		bool	bExactPositioning;
		PATHPOINTVECTOR	remainingPath;
		float	fDistanceToPathEnd;

		SMovementRequestParams()
		{
			aimLook = false;
			bodystate = 0;
			vLookTargetPos.zero(); vAimTargetPos.zero(); vLookTargetPos.zero(); vMoveDir.zero();
			fDesiredSpeed = fDistanceToPathEnd = 0.0f;
		};

		/// Initializes SMovementRequestParams from CMovementRequest.
		explicit SMovementRequestParams (CMovementRequest& );
	};

	CSpotter();
	virtual ~CSpotter();

	//CActor
	virtual void SetAuthority(bool auth ) {};
	virtual void SerializeXML(XmlNodeRef& node, bool bLoading ) {};
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void ProcessEvent(SEntityEvent& event);
	virtual void Update(SEntityUpdateContext&, int updateSlot);
	virtual void PrePhysicsUpdate();

	//Spotter parameters
	virtual void PostPhysicalize();
	virtual void PostInit( IGameObject * pGameObject );
	virtual void RagDollize( bool fallAndPlay );
	virtual void Kill();

	//AI specific
	virtual void SetActorMovement(SMovementRequestParams &control);
	virtual void GetActorInfo( SBodyInfo& bodyInfo );
	virtual int	 GetBoneID(int ID,int slot = 0) const;
	
	virtual int	GetActorSpecies() const { return eGCT_SPOTTER; }

	//Movement controller
	virtual IActorMovementController * CreateMovementController();

	//Model data for body info
	ILINE const Vec3& GetWeaponOffset() const { return m_modelInfo.weaponOffset; }
	ILINE const Vec3& GetEyeOffset() const { return m_modelInfo.eyeOffset; }

	static  const char* GetActorClassType() { return "CSpotter"; }
	virtual const char* GetActorClass() const { return CSpotter::GetActorClassType(); }

	//Misc
	virtual void UpdateStats(float frameTime);
	virtual SActorStats *GetActorStats() { return &m_stats; };
	virtual const SActorStats *GetActorStats() const { return &m_stats; };

	//Actions
	virtual void PlayAction(const char *action,const char *extension, bool looping=false);
	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC);
	virtual bool CreateCodeEvent(SmartScriptTable &rTable);


	typedef enum ESpotterAnimation
	{
		eSA_Idle=0,
		eSA_AlertedIdle,
		eSA_Fly,
		eSA_AlertedFly,
		eSA_Fly_Up,
		eSA_Fly_Down,
		eSA_Strafe_Left,
		eSA_Strafe_Right,
		eSA_AlertedStart,
		eSA_Last
	} ESpotterAnimation;

	//~CActor

	void EMP(float radius, float duration);
	void Scan(float range, float fov);
	void EnergySensor(const Vec3 &pos, float awareness);
	void DrawSensor();

private:
	SSpotterModelInfo		m_modelInfo;
	SSpotterStats				m_stats;
	SMovementRequestParams m_movementRequest;

	static const char *m_animations[eSA_Last];

	Vec3 m_sensorPos;
	float m_sensorAwareness;
	Vec3 m_lastMoveDir;

	bool m_alerted;
	float m_scanTimer;
	float m_scanTick;
	float m_scanFov;
	float m_scanRange;

	float m_alertedStartTimer;

	std::vector<EntityId> m_scanned;

	void DebugValue(const char *id, float value);
	IDebugHistoryManager *m_pDebugHistoryManager;
};



#endif