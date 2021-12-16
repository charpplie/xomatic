/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Player nav path helper

-------------------------------------------------------------------------
History:
- 02:11:2009: Created by Kevin Kirst
- 18:11:2009: Made Nav Path code based on Kevin's Ruler code

*************************************************************************/

#ifndef __PLAYERNAVPATH_H__
#define __PLAYERNAVPATH_H__

#include "PlayerNavPathPoint.h"
#include "PlayerNavPathAgent.h"
#include "PlayerNavPathGraph.h"
#include "IPlayerUpdateListener.h"
#include "../../CryEngine/CryAISystem/GenericAStarSolver.h"

class CPlayer;

//! The PlayerNavPath utility helps player's visualize the optimal path to arbitrary locations on the map
class CPlayerNavPath : public IPlayerUpdateListener
{
public:

	typedef std::vector<Vec3> TPossibleConnections;

	CPlayerNavPath(CPlayer &player);
	~CPlayerNavPath();

	//! Returns if there is a path to draw
	bool HasValidPath() const;
	const CPlayerNavPathAgent::TNavPath& GetPath() const;
	const CPlayerNavPathAgent::TNavPath& GetPurePath() const;

	int GetClosestPointIndex() const { return m_iClosestPointIndex; }
	const Vec3& GetNextPurePathPoint() const;
	const CPlayerNavPath::TPossibleConnections& GetPossibleConnections() const;

	//! Set a point to path to
	void SetTarget(const EntityId targetEntityId);
	void SetTarget(const Vec3 &vTargetPoint);
	void SetPath(const char* path);
	void SetTagPointStart(const EntityId startId);
	void Clear();

	//! Information about target type
	bool TargetIsTeammateSpike() const { return !m_backupTargetPoint.IsEmpty(); }
	bool TargetIsEntity() const { return (m_targetPoint.GetTargetEntityID() != 0); }
	EntityId GetTargetEntityId() const { return m_targetPoint.GetTargetEntityID(); }
	
	// IPlayerUpdateListener
	virtual void Update(float fFrameTime);
	// ~IPlayerUpdateListener
	
	//! Reset
	void Reset();

private:
	
	//! Request a path using the path agent
	void RequestPath();

	void UpdateClosestPoint();
	void UpdateNextPurePathPoint(const int advance = 2);
	void UpdatePossibleConnections();

	CPlayer *m_pPlayer;		// guaranteed to not be NULL 
	int m_iClosestPointIndex;

	// Base point
	CPlayerNavPathPoint m_startPoint;
	CPlayerNavPathPoint m_targetPoint;
	CPlayerNavPathPoint m_backupTargetPoint; // to save target point when overriding with teammate spike position

	CPlayerNavPathGraph::SNavTagPoint m_startNavNode;
	CPlayerNavPathGraph::SNavTagPoint m_endNavNode;

	CPlayerNavPathGraph m_tagPointCloud;

	bool m_bRequestPending;
	bool m_bRequestStarted;

	Vec3 m_nextPurePathPoint;
	Vec3 m_currentPurePathPoint;
	float m_lastDistanceCheck;
	TPossibleConnections m_possibleConnections;

	GenericAStarSolver<CPlayerNavPathGraph, CPlayerNavPathHeuristics, CPlayerNavPathCostCalculator, Vec3> m_pathFinder;
	CPlayerNavPathHeuristics m_pathFinderHeuristics;

	std::vector<Vec3> m_path;
	std::vector<Vec3> m_purePath;

};

//////////////////////////////////////////////////////////////////////////
ILINE const CPlayerNavPathAgent::TNavPath& CPlayerNavPath::GetPath() const
{
	return m_purePath;
}

//////////////////////////////////////////////////////////////////////////
ILINE const CPlayerNavPathAgent::TNavPath& CPlayerNavPath::GetPurePath() const
{
	return m_purePath;
}

//////////////////////////////////////////////////////////////////////////
ILINE void CPlayerNavPath::Reset()
{
	Clear();
}

#endif //__PLAYERNAVPATH_H__
