/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Player nav path agent, to hook in to AI path finder
  
 -------------------------------------------------------------------------
  History:
  - 02:11:2009: Created by Kevin Kirst
	- 18:11:2009: Made Nav Path code based on Kevin's Ruler code

*************************************************************************/

#ifndef __PLAYERNAVPATHAGENT_H__
#define __PLAYERNAVPATHAGENT_H__

#include "IAgent.h"
#include "IPathfinder.h"
#include "PlayerNavPathTagPointCloud.h"

class CPlayerNavPathPoint;

//! Player Nav Path Agent - wrapper for making path requests to the AI system
class CPlayerNavPathAgent : public IAIPathAgent
{
public:
	struct SSmoothedNavPathPoint
	{
		Vec3 vPoint;
		Vec3 vNormal;
	};

	struct SShapePath
	{

		typedef std::list<Vec3> TPositionList;

		SShapePath(const Vec3* points, const uint32 numpoints)
		{
			for(uint32 i=0; i<numpoints; ++i)
			{
				shape.push_back(points[i]);
			}
		}

		bool Empty() const
		{
			return shape.empty();
		}

		int Size() const
		{
			return shape.size();
		}

		Vec3 GetPosAlongPath(float dist) const
		{
			if (shape.empty())
				return ZERO;

			if (dist < 0.0f)
				return shape.front();

			TPositionList::const_iterator cur = shape.begin();
			TPositionList::const_iterator next(cur);
			++next;

			float d = 0.0f;
			while (next != shape.end())
			{
				Vec3 delta = *next - *cur;
				float len = delta.GetLength();
				if (len > 0.0f && dist >= d && dist < d+len)
				{
					float t = (dist - d) / len;
					return *cur + delta * t;
				}
				d += len;
				cur = next;
				++next;
			}
			return *cur;
		}

		float GeDistanceToPoint(int index) const
		{
			if (shape.empty())
				return 0.0f;

			if (index <= 0)
				return 0.0f;

			TPositionList::const_iterator cur = shape.begin();
			TPositionList::const_iterator next(cur);
			++next;

			int count = 0;
			float d = 0.0f;
			while (next != shape.end() && count<index)
			{
				Vec3 delta = *next - *cur;
				float len = delta.GetLength();
				d += len;
				cur = next;
				++next;
				++count;
			}
			return d;
		}

		Vec3 GetPoint(int index) const
		{
			if(index<0 || index>=shape.size())
				return ZERO;

			TPositionList::const_iterator cur = shape.begin();
			for(int i=0; i<index; ++i)
			{
				++cur;
			}
			return *cur;
		}

		float GetPathLength() const
		{
			if (shape.empty())
				return 0.0f;

			TPositionList::const_iterator cur = shape.begin();
			TPositionList::const_iterator next(cur);
			++next;

			float d = 0.0f;
			while (next != shape.end())
			{
				Vec3 delta = *next - *cur;
				float len = delta.GetLength();
				d += len;
				cur = next;
				++next;
			}
			return d;
		}

		TPositionList shape;
	};

	typedef std::vector<Vec3> TNavPath;
	typedef std::vector<SSmoothedNavPathPoint> TSmoothedNavPath;

	CPlayerNavPathAgent();
	virtual ~CPlayerNavPathAgent();
	
	void Reset();

	void SetPlayerEntity(IEntity* pPlayerEntity);

	// Request a path
	bool RequestPath(const CPlayerNavPathPoint &startPoint, const CPlayerNavPathPoint &endPoint);
	bool RequestPath(const char* designerPathName);
	bool RequestPath(const CPlayerNavPathTagPointCloud::SNavTagPoint& startNavNode, const CPlayerNavPathTagPointCloud::SNavTagPoint& endNavNode);
	void ClearLastRequest();
	bool HasQueuedPaths() const { return m_bPathQueued; }
	bool HasValidPath() const { return !m_path.empty(); }
	
	const TNavPath& GetPath() const { return m_path; }
	const TNavPath& GetPurePath() const { return m_purePath; }
	const TSmoothedNavPath& GetSmoothedPath() const { return m_smoothedPath; }
	Vec3 GetFirstPathPoint() const;
	Vec3 GetEndPathPoint() const;

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
	void GeneratePath(const INavPath *pPath);
	void GeneratePath(const SShapePath& pPath);
	int CalcNumSteps(float fPathDistance, int iMaxSteps, float fStepIncrease, float &fInitialLength) const;
	void AdjustPathPointHeight(Vec3 &vPoint, IAISystem::ENavigationType nextPointType) const;
	void MakeSmoothedPath();

	bool m_bPathQueued;
	bool m_bLastPathSuccess;
	unsigned int m_LastNavNode;
	float m_fLastPathDist;

	Vec3 m_vStartPoint;
	AgentMovementAbility m_AgentMovementAbility;
	TNavPath m_path;
	TNavPath m_purePath;
	TSmoothedNavPath m_smoothedPath;
	IEntity* m_pPlayerEntity;
};

#endif //__PLAYERNAVPATHAGENT_H__
