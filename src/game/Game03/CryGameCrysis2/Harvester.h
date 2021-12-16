#ifndef __HARVESTER_H__
#define __HARVESTER_H__

#pragma once


#include "Actor.h"
#include "GameUtils.h"



//TODO prune out crap not needed copied from pinger.
struct SHarvesterStats : public SActorStats
{
	SHarvesterStats()
	{
		memset(this,0,sizeof(SHarvesterStats));	//<-This should be bad! so WTF? Copied from elsewhere
	}
	void Serialize( TSerialize ser );
};



class CHarvester : public CActor
{
private:

	typedef enum ESpotterAnimation
	{
		ANIM_MOVEMENT,
		ANIM_IDLE,
		ANIM_WALK,
		ANIM_RUN,
		ANIM_JUMP,
		ANIM_IDLE_AFRAID,
		ANIM_DRINKSPILL,

		ANIM_NORMAL_MAX,
		ANIM_FULL_BEGIN = ANIM_NORMAL_MAX,

		ANIM_MOVEMENT_FULL = ANIM_FULL_BEGIN,
		ANIM_IDLE_FULL,
		ANIM_WALK_FULL,
		ANIM_RUN_FULL,
		ANIM_JUMP_FULL,
		ANIM_IDLE_AFRAID_FULL,
		ANIM_DRINKSPILL_FULL,

		ANIM_FULL_MAX,
		
		ANIM__MAX = ANIM_FULL_MAX
	} ESpotterAnimation;
	static const char *m_animations[ANIM__MAX];
	const char* m_currentAnim, *m_lastAnim;
	bool m_bIgnoreMovementRequests;
	bool m_bAnimationEnabled;
	bool m_Full;
	EntityId m_target;

	//Why is this not part of the actor class? or some intermediate pawn?
	struct SHarvesterModelInfo
	{
		Vec3		weaponOffset;
		Vec3		eyeOffset;

		Matrix33	viewMtx;  //Actual view direction
		Matrix33	baseMtx;	//base rotation matrix, rotation on the Z axis

		SHarvesterModelInfo() :
		weaponOffset(ZERO), eyeOffset(ZERO)
		{
			baseMtx.SetIdentity();
			viewMtx.SetIdentity();
		}
	};

public:

	struct SHarvesterBodyInfo {
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

		SHarvesterBodyInfo() : vEyePos(ZERO), vEyeDir(ZERO), vEyeDirAnim(ZERO),
			vFwdDir(ZERO), vUpDir(ZERO), vFireDir(ZERO),
			maxSpeed(0), normalSpeed(0), minSpeed(0),
			stance(STANCE_NULL)
		{
			m_stanceSizeAABB.min	=Vec3(ZERO);
			m_stanceSizeAABB.max	=Vec3(ZERO);
			m_colliderSizeAABB.min	=Vec3(ZERO);
			m_colliderSizeAABB.max	=Vec3(ZERO);
		}
	};

	struct SHarvesterMovementRequestParams
	{
		bool	aimLook;
		int		bodystate;
		Vec3	vShootTargetPos;	//TODO relevant?
		Vec3	vAimTargetPos;	//TODO relevant?
		Vec3	vLookTargetPos;
		Vec3	vMoveDir;
		float	fDesiredSpeed;
		EActorTargetPhase	eActorTargetPhase;
		bool	bExactPositioning;
		PATHPOINTVECTOR	remainingPath;
		float	fDistanceToPathEnd;

		SHarvesterMovementRequestParams()
		{
			aimLook = false;
			bodystate = 0;
			vLookTargetPos.zero(); vAimTargetPos.zero(); vLookTargetPos.zero(); vMoveDir.zero();
			fDesiredSpeed = fDistanceToPathEnd = 0.0f;
		};

		// Initializes SMovementRequestParams from CMovementRequest.
		explicit SHarvesterMovementRequestParams (CMovementRequest& );
	};


private:

	SHarvesterStats			m_HarvesterStats;
	SHarvesterModelInfo		m_HarvesterModelInfo;
	SHarvesterMovementRequestParams	m_HarvesterMovementRequestParams;
	IDebugHistoryManager *m_pDebugHistoryManager;


public:

	CHarvester();
	virtual ~CHarvester();

	//CActor
	virtual void SetAuthority(bool auth ) {};
	virtual void SerializeXML(XmlNodeRef& node, bool bLoading ) {};
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void ProcessEvent(SEntityEvent& event);
	virtual void Update(SEntityUpdateContext&, int updateSlot);
	virtual void PrePhysicsUpdate();

	//Harvester parameters
	virtual void PostPhysicalize();
	virtual void PostInit( IGameObject * pGameObject );
	virtual void RagDollize( bool fallAndPlay );
	virtual void Kill();

	//AI specific
	virtual void SetActorMovement(SHarvesterMovementRequestParams &control);
	virtual void GetActorInfo( SHarvesterBodyInfo& bodyInfo );
	//virtual int	 GetBoneID(int ID,int slot = 0) const;

	virtual int	GetActorSpecies() const { return eGCT_HARVESTER; }

	virtual bool CreateCodeEvent(SmartScriptTable &rTable);

	//Movement controller
	virtual IActorMovementController * CreateMovementController();

	//Model data for body info
	ILINE const Vec3& GetWeaponOffset() const { return m_HarvesterModelInfo.weaponOffset; }
	ILINE const Vec3& GetEyeOffset() const { return m_HarvesterModelInfo.eyeOffset; }

	static  const char* GetActorClassType() { return "CHarvester"; }
	virtual const char* GetActorClass() const { return CHarvester::GetActorClassType(); }

	//Misc
	virtual void UpdateStats(float frameTime);
	virtual SActorStats *GetActorStats() { return &m_HarvesterStats; };
	virtual const SActorStats *GetActorStats() const { return &m_HarvesterStats; };

	//Actions
	virtual void PlayAction(const char *action,const char *extension, bool looping=false);
	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC);

	void DebugValue(const char *id, float value);

};


///////////TEMP
class CHarvesterMovementController : public IActorMovementController
{
public:

	CHarvesterMovementController(CHarvester* pHarvester);
	~CHarvesterMovementController();

	//IActorMovementController
	virtual void Reset(){};
	virtual bool Update( float frameTime, SActorFrameMovementParams& params );
	virtual bool GetStats(SStats& stats){ return false; };
	virtual void PostUpdate( float frameTime ){};
	virtual void Release();
	virtual void BindInputs( IAnimationGraphState * ) {};
	virtual void Serialize(TSerialize &ser){};
	//~IActorMovementController

	//IMovementController
	virtual bool RequestMovement( CMovementRequest& request );
	ILINE virtual void GetMovementState( SMovementState& state ) { state = m_currentMovementState; };
	virtual bool GetStanceState( const SStanceStateQuery& query, SStanceState& state );
	//~IMovementController

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const;
private:

	CHarvester*	m_pHarvester;

	CMovementRequest m_currentMovementRequest;
	SMovementState m_currentMovementState;
};
////////////TEMP




#endif