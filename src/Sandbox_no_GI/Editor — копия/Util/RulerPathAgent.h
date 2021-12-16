/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Ruler path agent, to hook in to AI path finder
  
 -------------------------------------------------------------------------
  History:
  - 02:11:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __RULERPATHAGENT_H__
#define __RULERPATHAGENT_H__

#include "IAgent.h"
#include "IPathfinder.h"

class CRuler;
class CRulerPoint;

//! Ruler point helper - Defines a point for the ruler
class CRulerPathAgent : public IAIPathAgent
{
public:
	CRulerPathAgent();
	virtual ~CRulerPathAgent();
	
	void Init(CRuler *pRuler);
	void Render(IRenderer *pRenderer, const ColorF& colour);

	void SetType(uint32 type, const string& name);
	uint32 GetType() const { return m_agentType; }
	const string& GetName() const { return m_agentName; }

	// Request a path
	bool RequestPath(const CRulerPoint &startPoint, const CRulerPoint &endPoint);
	void ClearLastRequest();
	bool HasQueuedPaths() const { return m_bPathQueued; }

	// Get path output
	bool GetLastPathSuccess() const { return m_bLastPathSuccess; }
	float GetLastPathDist() const { return m_fLastPathDist; }

	// IAIPathAgent
	virtual IEntity *GetPathAgentEntity() const;
	virtual const char *GetPathAgentName() const;
	virtual unsigned short GetPathAgentType() const;

	virtual float GetPathAgentPassRadius() const;
	virtual Vec3 GetPathAgentPos() const;
	virtual Vec3 GetPathAgentVelocity() const;

	virtual const AgentMovementAbility &GetPathAgentMovementAbility() const;
	virtual void PathEvent(SAIEVENT *pEvent);

	virtual void GetPathAgentNavigationBlockers(NavigationBlockers &blockers, const PathfindRequest *pRequest);

	virtual unsigned int GetPathAgentLastNavNode() const;
	virtual void SetPathAgentLastNavNode(unsigned int lastNavNode);
	virtual Vec3 GetForcedStartPos() const;

	virtual void SetPathToFollow( const char* pathName ){}
	virtual void SetPathAttributeToFollow( bool bSpline ){}
	virtual void SetPointListToFollow( const std::list<Vec3>& pointList,IAISystem::ENavigationType navType,bool bSpline ){}

	//Path finding avoids blocker type by radius. 
	virtual void SetPFBlockerRadius(int blockerType, float radius){}


	//Can path be modified to use request.targetPoint?  Results are cacheded in request.
	virtual ETriState CanTargetPointBeReached(CTargetPointRequest& request){ETriState garbage = eTS_maybe; return garbage;}

	//Is request still valid/use able
	virtual bool UseTargetPointRequest(const CTargetPointRequest& request){return false;}//??

	virtual bool GetValidPositionNearby(const Vec3& proposedPosition, Vec3& adjustedPosition) const{return false;}
	virtual bool GetTeleportPosition(Vec3& teleportPos) const{return false;}
	//~IAIPathAgent

private:
	bool m_bPathQueued;
	bool m_bLastPathSuccess;
	unsigned int m_LastNavNode;
	float m_fLastPathDist;

	Vec3 m_vStartPoint;
	AgentMovementAbility m_AgentMovementAbility;
	uint32 m_agentType;
	string m_agentName;

	PATHPOINTVECTOR	m_path;	// caches the path locally to allow multiple requests
};

#endif //__RULERPATHAGENT_H__
