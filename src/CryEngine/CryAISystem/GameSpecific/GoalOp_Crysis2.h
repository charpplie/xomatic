/******************************************************************** 
StalTech Source File.
Copyright (C), StalTech Studios, 2008-2009.
-------------------------------------------------------------------------
File name:   GoalOp_Crysis2.h
Description: Crysi2 GoalOps
             These should move into GameDLL when interfaces allow!
-------------------------------------------------------------------------
History:
- 18:11:2009 - Created by Márcio Martins
*********************************************************************/

#ifndef __GoalOp_Crysis2_H__
#define __GoalOp_Crysis2_H__

#pragma once

#include "GoalOp.h"
#include "GoalOpFactory.h"
#include "Communication/Communication.h"

// Forward declarations
class COPPathFind;
class COPTrace;

/**
* Factory for Crysis2 goalops
*
*/
class CGoalOpFactoryCrysis2 : public IGoalOpFactory
{
	IGoalOp * GetGoalOp( const char * sGoalOpName, IFunctionHandler *pH, int nFirstParam, GoalParameters &params ) const;
	IGoalOp * GetGoalOp( EGoalOperations op, GoalParameters &params ) const;
};

////////////////////////////////////////////////////////////
//
//			USECOVER - hide/unhide at current hide spot
//
////////////////////////////////////////////////////////

struct CoverUsageInfo;

class COPCrysis2UseCover : public CGoalOp
{
public:
	typedef enum ECoverMoveState
	{
		eCMS_None = 0,
		eCMS_Moving,
	};

	typedef enum ECoverUsage
	{
		eCU_Hide = 0,
		eCU_UnHide,
	};

	typedef enum ECoverHeight
	{
		eCH_None = 0,
		eCH_Low,
		eCH_High
	};

	COPCrysis2UseCover(ECoverUsage usage, ECoverUsageLocation location, bool useLastOpAsBackup);
	COPCrysis2UseCover(const XmlNodeRef& node);
	virtual ~COPCrysis2UseCover();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void DebugDraw(CPipeUser* pPipeUser) const;
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
protected:
	EGoalOpResult UseCover(CPipeUser* pPipeUser);

	float ScoreByLocationDistance(CPipeUser* pPipeUser, const Vec3& pos, const Vec3& locationPos) const;
	float ScoreByDistanceToTarget(CPipeUser* pPipeUser, const Vec3& locationPos, const Vec3& target) const;
	float ScoreByAngle(CPipeUser* pPipeUser, const Vec3& locationPos, const Vec3& target) const;
	float ScoreByCoverage(CPipeUser* pPipeUser, bool hasLowCover, bool hasHighCover, ECoverUsage coverUsage, ECoverUsageLocation location) const;

	const Vec3& GetCoverLocationPosition(ECoverUsageLocation location) const;

	IAIObject* GetTarget(CPipeUser* pPipeUser) const;
	void UpdateCoverLocations(CPipeUser* pPipeUser, bool useLowCover, const Vec3& target, ECoverUsage coverUsage);
	float GetCoverLocationScore(CPipeUser* pPipeUser, const CoverUsageInfo& coverUsageInfo, ECoverUsage coverUsage, ECoverUsageLocation location) const;
	ECoverUsageLocation GetAutomaticCoverLocationChoice(CPipeUser* pPipeUser, const CoverUsageInfo& coverUsageInfo);
	ECoverHeight GetCoverHeight(CPipeUser* pPipeUser, const Vec3& target, const Vec3& from, const Vec3& to);
	ECoverHeight GetCoverHeightAt(CPipeUser* pPipeUser, const Vec3& target, const Vec3& pos);
	
	void UpdateInvalidSeg(CPipeUser* pPipeUser);

	bool m_useLastOpAsBackup;
	
	ECoverUsage m_coverUsage;
	ECoverUsageLocation m_coverUsageLocation;
	ECoverMoveState	m_coverMoveState;
	
	bool				m_targetReached;
	bool				m_coverCompromised;

	Vec3				m_center;
	Vec3				m_leftEdge;
	Vec3				m_rightEdge;

	Vec3				m_pose;
	Vec3				m_moveTarget;

	CTimeValue	m_moveStartTime;

	CTimeValue	m_leftInvalidStartTime;
	float				m_leftInvalidDist;
	Vec3				m_leftInvalidPos;

	CTimeValue	m_rightInvalidStartTime;
	float				m_rightInvalidDist;
	Vec3				m_rightInvalidPos;

	COPTrace*		m_pTracer;

	struct CoverLocationScore
	{
		CoverLocationScore(ECoverUsageLocation _location, float _score)
			: location(_location)
			, score(_score)
		{
		}

		ECoverUsageLocation location;
		float score;

		bool operator <(const CoverLocationScore& other) const
		{
			return score > other.score;
		}
	};

	typedef std::vector<CoverLocationScore> LocationScores;
	LocationScores m_locationScores;
};



////////////////////////////////////////////////////////
//
//	Adjust aim while staying still.
//
////////////////////////////////////////////////////////
class COPCrysis2AdjustAim: public CGoalOp
{
	CTimeValue	m_startTime;
	bool				m_hide;
	bool				m_useLastOpAsBackup;
	bool				m_allowProne;
	float				m_timeOut;
	float				m_nextUpdate;
	int					m_bestPostureId;

	float	RandomizeTimeInterval();

public:
	COPCrysis2AdjustAim(bool hide, bool useLastOpAsBackup, bool allowProne, float timeOut);
	COPCrysis2AdjustAim(const XmlNodeRef& node);

	virtual EGoalOpResult Execute(CPipeUser* pPipeUser);
	virtual void Reset( CPipeUser* pPipeUser );
	virtual void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
	virtual void DebugDraw(CPipeUser* pPipeUser) const;
};



////////////////////////////////////////////////////////////
//
//	HIDE - makes agent find closest hiding place and then hide there
//
////////////////////////////////////////////////////////
class COPCrysis2Hide : public CGoalOp
{
public:
	COPCrysis2Hide(EAIRegister location, bool exact);
	COPCrysis2Hide(const XmlNodeRef& node);
	virtual ~COPCrysis2Hide();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
private:
	void CreateHideTarget(const char* name, const Vec3 &pos);

	CStrongRef<CAIObject> m_refHideTarget;
	EAIRegister m_location;
	bool m_exact;

	COPPathFind *m_pPathfinder;
	COPTrace *m_pTracer;
};


////////////////////////////////////////////////////////////
//
//	COMMUNICATE - makes agent communicate (duh!)
//
////////////////////////////////////////////////////////
class COPCrysis2Communicate : public CGoalOp
{
public:
	COPCrysis2Communicate(CommunicationID commID, CommunicationChannelID channelID, float expirity, EAIRegister target,
		SCommunicationRequest::EOrdering ordering);
	COPCrysis2Communicate(const XmlNodeRef& node);
	virtual ~COPCrysis2Communicate();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
private:
	CommunicationID m_commID;
	CommunicationChannelID m_channelID;
	SCommunicationRequest::EOrdering m_ordering;

	float m_expirity;
	EAIRegister m_target;
};


#endif //__GoalOp_Crysis2_H__
