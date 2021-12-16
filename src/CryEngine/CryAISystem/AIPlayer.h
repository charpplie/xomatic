/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   AIPlayer.h
$Id$
Description: 

-------------------------------------------------------------------------
History:
- ?
- 2 Mar 2009			 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _AI_PLAYER_
#define _AI_PLAYER_

#if _MSC_VER > 1000
#pragma once
#endif

#include "AIActor.h"

class CObjectTracker;

class CMissLocationSensor
{
	struct MissLocation
	{
		enum EType
		{
			Destroyable,
			Rope,
			ManuallyBreakable,
			JointStructure,
			Deformable,
			MatBreakable,
			Unbreakable,
		};

		MissLocation()
		{
		}

		MissLocation(const Vec3& pos, EType typ)
			: position(pos)
			, type(typ)
			, score(0.0f)
		{
		}

		bool operator <(const MissLocation& rhs) const
		{
			return score < rhs.score;
		};

		Vec3 position;
		float score;
		EType type;
	};

	enum EState
	{
		Starting = 0,
		Collecting,
		Filtering,
		Finishing,
	};

	enum
	{
		MaxCollectedCount = 512,
		MaxConsiderCount = 64,
		MaxRopeVertexCount = 4,
		MaxRandomPool = 6,
	};

public:
	CMissLocationSensor(const CAIActor* owner);

	void Update(float timeLimit);
	void Collect(int objTypes);
	bool Filter(float timeLimit);
	bool GetLocation(const Vec3& shootPos, const Vec3& shootDir, float maxAngle, Vec3& pos);

	void AddDestroyableClass(const char* className);
	void ResetDestroyableClasses();

public:
	typedef std::vector<IPhysicalEntity*> MissEntities;
	MissEntities m_entities;

	typedef std::vector<MissLocation> MissLocations;
	MissLocations m_locations;
	MissLocations m_working;
	
	MissLocations m_goodies; 
	typedef std::vector<Vec3> RopeVertices;
	RopeVertices m_vertices;

	typedef std::vector<IEntityClass*> DestroyableClasses;
	DestroyableClasses m_destroyableEntityClasses;

	uint32 m_updateCount;
	EState m_state;
	const CAIActor* m_owner;
};


class CAIPlayer :	public CAIActor
{
public:
	CAIPlayer();
	~CAIPlayer();

	void ParseParameters(const AIObjectParameters & params);
	void Reset(EObjectResetType type);
	
	CWeakRef<CAIObject> GetAttentionTarget() const {return m_refAttentionTarget;}
	void UpdateAttentionTarget(CWeakRef<CAIObject> refTarget) ;
	virtual EFieldOfViewResult IsPointInFOV(const Vec3& pos, float distanceScale = 1.0f) const;
	void	Update(EObjectUpdate type);
	virtual bool Serialize(TSerialize ser, class CObjectTracker& objectTracker );
	virtual void OnObjectRemoved(CAIObject *pObject);
	virtual bool IsLowHealthPauseActive() const;
	virtual IEntity* GetGrabbedEntity() const;// consider only player grabbing things, don't care about NPC

	// Inherited
	virtual bool	IsAgent() const { return true; }
	// Inherited
	virtual DamagePartVector*	GetDamageParts();

	virtual void Event(unsigned short eType, SAIEVENT *pEvent);

	virtual IPhysicalEntity* GetPhysics(bool wantCharacterPhysics) const;

	int	GetDeathCount() { return m_deathCount; }
	void	IncDeathCount() { m_deathCount++; }

	virtual void	RecordEvent(IAIRecordable::e_AIDbgEvent event, const IAIRecordable::RecorderEventData* pEventData=NULL);
	virtual void	RecordSnapshot();

	virtual float AdjustTargetVisibleRange(const CAIActor& observer, float fVisibleRange) const;

	bool IsThrownByPlayer(EntityId ent) const;
	bool IsPlayerStuntAffectingTheDeathOf(CAIActor* pDeadActor) const;
	EntityId GetNearestThrownEntity(const Vec3& pos);
	bool IsDoingStuntActionRelatedTo(const Vec3& pos, float nearDistance);

	virtual bool	IsActive() const { return m_bEnabled; }

	virtual void GetPhysicsEntitiesToSkip(std::vector<IPhysicalEntity*>& skips) const;

	bool GetMissLocation(const Vec3& shootPos, const Vec3& shootDir, float maxAngle, Vec3& pos);
	void DebugDraw();

private:
	// (MATT) Note: this is a different attention target to the classic. Nasty OO workaround {2009/02/03}
	CWeakRef<CAIObject> m_refAttentionTarget;

	CTimeValue m_fLastUpdateTargetTime;
	float	m_FOV;
	DamagePartVector	m_damageParts;
	int	m_deathCount;

	struct SThrownItem
	{
		SThrownItem(EntityId id) : id(id), time(0.0f), pos(0,0,0), vel(0,0,0), r(0.1f) {}
		inline bool operator<(const SThrownItem& rhs) const { return time < rhs.time; }
		float time;
		Vec3 pos, vel;
		float r;
		EntityId id;
	};
	std::vector<SThrownItem>	m_lastThrownItems;

	struct SStuntTargetPuppet
	{
		SStuntTargetPuppet(CPuppet* pPuppet, const Vec3& pos) : pPuppet(pPuppet), t(0), exposed(0), signalled(false), threatPos(pos) {}
		CPuppet* pPuppet;
		Vec3 threatPos;
		float t;
		float exposed;
		bool signalled;
	};
	std::vector<SStuntTargetPuppet> m_stuntTargets;

	void AddThrownEntity(EntityId id);

	void UpdatePlayerStuntActions();
	void HandleCloaking(bool cloak);

	Vec3 m_stuntDir;

	float m_playerStuntSprinting;
	float m_playerStuntJumping;
	float m_playerStuntCloaking;
	float m_playerStuntUncloaking;

	float m_mercyTimer;

	void CollectExposedCover();
	void ReleaseExposedCoverObjects();
	void AddExposedCoverObject(IPhysicalEntity* pPhysEnt);
	void CollectExposedCoverRayComplete(const QueuedRayID& rayID, const RayCastResult& result);

	float m_coverExposedTime;

	struct SExposedCoverObject
	{
		SExposedCoverObject(IPhysicalEntity* pPhysEnt, float t) : pPhysEnt(pPhysEnt), t(t) {}
		IPhysicalEntity* pPhysEnt;
		float t;
	};

	struct ExposedCoverState
	{
		ExposedCoverState() : asyncState(AsyncReady), rayID(0) {}
		AsyncState asyncState;
		QueuedRayID rayID;
	};

	std::vector<SExposedCoverObject> m_exposedCoverObjects;
	ExposedCoverState m_exposedCoverState;

	CMissLocationSensor m_missLocationSensor;
};

inline const CAIPlayer* CastToCAIPlayerSafe(const IAIObject* pAI) { return pAI ? pAI->CastToCAIPlayer() : 0; }
inline CAIPlayer* CastToCAIPlayerSafe(IAIObject* pAI) { return pAI ? pAI->CastToCAIPlayer() : 0; }

#endif
