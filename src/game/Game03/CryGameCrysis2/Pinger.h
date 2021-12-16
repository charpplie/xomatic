//-----------------------------------------------
// Pinger prototype
//
// 11-08-08: Benito G.R.
//----------------------------------------------

#ifndef __PINGER_H__
#define __PINGER_H__

#pragma once

#include "Actor.h"
#include "GameUtils.h"
#include "Pinger_Mechanics.h"

struct SPingerStats : public SActorStats
{
	SPingerStats()
	{
		memset(this,0,sizeof(SPingerStats));
	}
	void Serialize( TSerialize ser );

};

// -- CPinger ------------------------------
class CPinger : public CActor
{
private:

	struct SPingerModelInfo
	{
		Vec3		weaponOffset;
		Vec3		eyeOffset;

		Matrix33  viewMtx;  //Actual view direction
		Matrix33	baseMtx;	//base rotation matrix, rotation on the Z axis

		SPingerModelInfo() :
			weaponOffset(ZERO), eyeOffset(ZERO)
		{
			baseMtx.SetIdentity();
			viewMtx.SetIdentity();
		}
	};

public:

	struct SBodyInfo {
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

	struct mi_pinger_params : public mi_params
	{
		CPinger::SMovementRequestParams moveRequest;

		mi_pinger_params()
		{
			type = eMIP_PINGER;
		}
	};

	struct mi_pinger_state : public mi_state
	{
		float speed;
		Vec3	velocity;
		Vec3  angVelocity;
		int   turningState;
		int   strafingState;

		mi_pinger_state()
		{
			type = eMIP_PINGER;
			speed = 0.0f;
			velocity.zero(); angVelocity.zero();
			turningState = 0;
			strafingState = 0;
		}
	};

	CPinger();
	virtual ~CPinger();

	//CActor
	virtual void SetAuthority(bool auth ) {};
	virtual void SerializeXML(XmlNodeRef& node, bool bLoading ) {};
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void ProcessEvent(SEntityEvent& event);
	virtual void Update(SEntityUpdateContext&, int updateSlot);
	virtual void PrePhysicsUpdate();

	virtual int GetActorSpecies() const { return eGCT_PINGER; };

	//Pinger parameters
	virtual void PostPhysicalize();
	virtual void PostInit( IGameObject * pGameObject );
	virtual void RagDollize( bool fallAndPlay );
	virtual void Kill();

	//AI specific
	virtual void SetActorMovement(SMovementRequestParams &control);
	virtual void GetActorInfo( SBodyInfo& bodyInfo );
	virtual int	 GetBoneID(int ID,int slot = 0) const;
	
	//Movement controller
	virtual IActorMovementController * CreateMovementController();

	//Model data for body info
	ILINE const Vec3& GetWeaponOffset() const { return m_modelInfo.weaponOffset; }
	ILINE const Vec3& GetEyeOffset() const { return m_modelInfo.eyeOffset; }

	static  const char* GetActorClassType() { return "CPinger"; }
	virtual const char* GetActorClass() const { return CPinger::GetActorClassType(); }

	//Misc
	virtual void UpdateStats(float frameTime);
	virtual SActorStats *GetActorStats() { return &m_stats; };
	virtual const SActorStats *GetActorStats() const { return &m_stats; };

	//Actions
	virtual void PlayAction(const char *action,const char *extension, bool looping=false);
	virtual void RequestMechanic(const char* mechanicName);
	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC);

	//~CActor

private:

	void RequestDefaultMechanic();

	SPingerModelInfo		m_modelInfo;
	SPingerStats				m_stats;

	IMyMechanic					*m_pCurrentMechanic;
	mi_pinger_params		 m_mechanicParams;
	mi_pinger_state			 m_pingerState;
};



#endif