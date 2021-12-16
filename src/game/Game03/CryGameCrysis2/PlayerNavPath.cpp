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

#include "StdAfx.h"
#include "PlayerNavPath.h"
#include "Player.h"
#include "GameRules.h"
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"

//////////////////////////////////////////////////////////////////////////
CPlayerNavPath::CPlayerNavPath(CPlayer &player)
: m_pPlayer(&player)
, m_iClosestPointIndex(-1)
, m_bRequestPending(false)
, m_bRequestStarted(false)
, m_lastDistanceCheck(0.0f)
{
	m_startPoint.Set(m_pPlayer->GetEntityId());
	m_nextPurePathPoint = m_startPoint.GetPos();
	m_currentPurePathPoint.zero();
	m_pPlayer->RegisterPlayerUpdateListener(this);
}

//////////////////////////////////////////////////////////////////////////
CPlayerNavPath::~CPlayerNavPath()
{
	m_pPlayer->UnregisterPlayerUpdateListener(this);
}

//////////////////////////////////////////////////////////////////////////
bool CPlayerNavPath::HasValidPath() const
{
	return (g_pGame->GetHUD()->GetCVars()->hud_navPath_enable!=0 && !m_targetPoint.IsEmpty() && m_purePath.size());
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::Update(float)
{
	if (g_pGame->GetHUD()->GetCVars()->hud_navPath_enable==0 || m_targetPoint.IsEmpty())
		return;

	UpdateClosestPoint();

	if (m_bRequestPending)
	{
		RequestPath();
	}
	else if (m_iClosestPointIndex >= 0)
	{
		Vec3 startPoint = m_startPoint.GetPos();
		const int bestIndex = m_tagPointCloud.GetBestNodeIndex(startPoint);
		const Vec3 bestPos = m_tagPointCloud.GetNodeReferencePoint(bestIndex);
		
		const CPlayerNavPathAgent::TNavPath &purePath = GetPurePath();
		if(!stl::find(purePath, bestPos))
		{
			m_lastDistanceCheck = FLT_MAX;
			SetTarget(m_targetPoint.GetPos());
		}
	}
	else
	{
		RequestPath();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::UpdateClosestPoint()
{
	const CPlayerNavPathAgent::TNavPath &path = GetPath();
	const int iSize = path.size();
	const Vec3 vPlayerPos = m_pPlayer->GetEntity()->GetWorldPos();

	int curClosestPoint = 0;

	float fDistanceCheck = FLT_MAX;
	for (int i=0; i+1 < iSize; ++i)
	{
		const float fTempCheck = (vPlayerPos - path[i]).len2();
		if ( fTempCheck < fDistanceCheck )
		{
			curClosestPoint = i;
			fDistanceCheck = fTempCheck;
		}
	}
	
	if(iSize==0)
		return;

	bool update = m_iClosestPointIndex != curClosestPoint;
	m_iClosestPointIndex = curClosestPoint;

	if(curClosestPoint==0)
	{
		const bool firstTime = m_lastDistanceCheck==FLT_MAX;

		const bool previouslyNearPath = m_lastDistanceCheck<9.0f;
		const bool currentlyNearPath = fDistanceCheck<9.0f;

		const bool hasChanged = previouslyNearPath != currentlyNearPath;

		if(firstTime || hasChanged)
		{
			UpdateNextPurePathPoint(currentlyNearPath ? 2 : 0);
		}
		m_lastDistanceCheck = fDistanceCheck;
	}
	else if(update)
	{
		UpdateNextPurePathPoint();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::UpdateNextPurePathPoint(const int advance /*= 2*/)
{
	const CPlayerNavPathAgent::TNavPath &path = GetPath();
	const CPlayerNavPathAgent::TNavPath &purePath = GetPurePath();

	const int iSize = path.size();
	const int purePathSize = purePath.size();

	for (int i=m_iClosestPointIndex+advance; i < iSize; ++i)
	{
		for (int j=0; j < purePathSize; ++j)
		{
			if(purePath[j] == path[i])
			{
				if(m_nextPurePathPoint!=purePath[j])
				{
					if(j>0)
						m_currentPurePathPoint = purePath[j-1];
					else
						m_currentPurePathPoint.zero();
					m_nextPurePathPoint = purePath[j];
				}
				return;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
const Vec3& CPlayerNavPath::GetNextPurePathPoint() const
{
	return m_nextPurePathPoint;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::UpdatePossibleConnections()
{
/*	m_possibleConnections.clear();

	if(m_currentPurePathPoint.IsZero())
		return;

	CPlayerNavPathTagPointCloud::SNavTagPoint currentPoint;
	m_tagPointCloud.GetClosestNavNode(m_currentPurePathPoint, currentPoint);

	CPlayerNavPathTagPointCloud::SNavTagPoint::TLinks::const_iterator it = currentPoint.m_links.begin();
	CPlayerNavPathTagPointCloud::SNavTagPoint::TLinks::const_iterator end = currentPoint.m_links.end();

	CPlayerNavPathTagPointCloud::SNavTagPoint run;
	m_possibleConnections.reserve(currentPoint.m_links.size());
	for(; it!=end; ++it)
	{
		m_tagPointCloud.GetNavNode((*it), run);
		m_possibleConnections.push_back(run.m_pos);
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
const CPlayerNavPath::TPossibleConnections& CPlayerNavPath::GetPossibleConnections() const
{
	return m_possibleConnections;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::RequestPath()
{
	if(!m_startNavNode.m_pos.IsZero() && !m_endNavNode.m_pos.IsZero())
	{
		if(!m_bRequestStarted)
		{
			m_purePath.clear();
			m_pathFinder.StartPathFind(&m_tagPointCloud, m_startNavNode.m_pos, m_endNavNode.m_pos, &m_pathFinderHeuristics);
			m_bRequestStarted = true;
		}

		if(m_pathFinder.Update(7))
		{
			m_pathFinder.GetPath(m_purePath);
			m_bRequestStarted = false;
			m_bRequestPending = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::SetTarget(EntityId targetEntityId)
{
	if (targetEntityId != 0)
	{
		m_targetPoint.Set(targetEntityId);
		m_bRequestPending = true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::SetTarget(const Vec3 &vTargetPoint)
{
	if (!vTargetPoint.IsZero())
	{
		m_targetPoint.Set(vTargetPoint);

		if(m_tagPointCloud.IsValid())
		{
			m_tagPointCloud.GetClosestNavNode(m_startPoint.GetPos(), m_startNavNode);
			m_tagPointCloud.GetClosestNavNode(vTargetPoint, m_endNavNode);
		}
		m_bRequestPending = true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::SetPath(const char* path)
{
	if (path && path[0])
	{
		m_targetPoint.Set(path);
		m_bRequestPending = true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::SetTagPointStart(EntityId startId)
{
	m_tagPointCloud.Recalculate(startId);
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPath::Clear()
{
	m_targetPoint.Reset();
	m_backupTargetPoint.Reset();
	m_iClosestPointIndex = -1;
	m_nextPurePathPoint = ZERO;
	m_startNavNode = CPlayerNavPathGraph::SNavTagPoint();
	m_endNavNode = CPlayerNavPathGraph::SNavTagPoint();
	m_bRequestPending = false;
	m_lastDistanceCheck = FLT_MAX;
}
