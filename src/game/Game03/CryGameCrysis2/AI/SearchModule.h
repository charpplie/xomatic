#pragma once

#ifndef SearchModule_h
#define SearchModule_h

#include "GameAISystem.h"
#include <IVisionMap.h>

struct SearchSpotQuery
{
	float closenessToAgentWeight;
	float closenessToTargetWeight;
	Vec3 result;
};

struct SearchActor
{
	SearchActor(EntityId _entityID, VisionID _visionID)
		: entityID(_entityID)
		, visionID(_visionID)
	{
	}

	EntityId entityID;
	VisionID visionID;
};

enum SearchSpotStatus
{
	NotSearchedYet,
	BeingSearchedRightAboutNow,
	Searched,
	Unreachable
};

class SearchSpot
{
public:
	SearchSpot();
	~SearchSpot();

	void Init(const Vec3& pos);
	void DebugDraw();
	void SetStatus(SearchSpotStatus newStatus) { m_status = newStatus; }
	SearchSpotStatus GetStatus() const { return m_status; }
	bool HasBeenSearched() const { return m_status == Searched; }
	bool IsBeingSearched() const { return m_status == BeingSearchedRightAboutNow; }
	const Vec3& GetPos() const { return m_pos; }
	operator const VisionID& () const { return m_visionID; }
	bool IsAssignedTo(EntityId entityID) const;
	void AssignTo(EntityId entityID);
	void MarkAsSearchedBy(SearchActor& participant);
	void MarkAsUnreachable();

//private:
	Vec3 m_pos;
	SearchSpotStatus m_status;
	VisionID m_visionID;
	EntityId m_assigneeID;
};

class SearchGroup
{
public:
	void Init(int groupID, const Vec3& targetPos);
	void Destroy();
	void Update();
	bool GetNextSearchPoint(EntityId entityID, SearchSpotQuery* query);
	void MarkAssignedSearchSpotAsUnreachable(EntityId entityID);

private:
	void StoreActors(int groupID);
	void GenerateSearchSpots();
	float CalculateScore(SearchSpot& searchSpot, EntityId entityID, SearchSpotQuery* query) const;
	SearchSpot* FindBestSearchSpot(EntityId entityID, SearchSpotQuery* query);
	SearchSpot* GetAssignedSearchSpot(EntityId entityID);

private:
	typedef std::vector<SearchSpot> SearchSpotContainer;
	typedef std::vector<SearchSpot>::iterator SearchSpotIter;

	typedef std::vector<SearchActor> SearchActorContainer;
	typedef std::vector<SearchActor>::iterator SearchActorIter;

	SearchActorContainer m_actors;
	SearchSpotContainer m_searchSpots;
	Vec3 m_targetPos;
};

class SearchModule : public IGameAIModule
{
public:
	static SearchModule* GetInstance();
	virtual void Enter(EntityId entityID);
	virtual void Leave(EntityId entityID);
	virtual void Pause(EntityId entityID) { Leave(entityID); }
	virtual void Resume(EntityId entityID) { Enter(entityID); }
	virtual void Reset();
	virtual void Update(float dt);
	virtual const char* GetName() const { return "SearchModule"; }

	void GroupEnter(int groupID, const Vec3& targetPos);
	void GroupLeave(int groupID);
	bool GetNextSearchPoint(EntityId entityID, SearchSpotQuery* query);
	void MarkAssignedSearchSpotAsUnreachable(EntityId entityID);

private:
	bool GroupExist(int groupID) const;
	SearchGroup* GetGroup(int groupID);
	void DebugDraw() const;

private:
	typedef int GroupID;
	typedef std::map<GroupID, SearchGroup> SearchGroupContainer;
	typedef std::map<GroupID, SearchGroup>::iterator SearchGroupIter;

	SearchGroupContainer m_groups;
};

#endif // SearchModule_h
